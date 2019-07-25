#include "cache-service-srv.h"

#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/udp-socket.h"
#include "ns3/socket-factory.h"
#include <ns3/tcp-socket-factory.h>

#include <ns3/random-variable-stream.h>
#include <ns3/tcp-socket.h>
#include <ns3/double.h>

#include "dash-server.h"
#include "http-header.h"
#include "mpeg-header.h"

namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("CacheService");
    NS_OBJECT_ENSURE_REGISTERED(CacheService);

    TypeId CacheService::GetTypeId(void)
    {
        static TypeId tid =
        TypeId("ns3::CacheService").SetParent<Application>().AddConstructor<CacheService>()
            .AddAttribute("Remote",
            "The address of the destination", AddressValue(),
            MakeAddressAccessor(&CacheService::m_peer), MakeAddressChecker())
            .AddAttribute("Local",
            "The address of the local peer", AddressValue(),
            MakeAddressAccessor(&CacheService::m_local), MakeAddressChecker())
            .AddAttribute("Protocol",
            "The type of TCP protocol to use.", TypeIdValue(TcpSocketFactory::GetTypeId()),
            MakeTypeIdAccessor(&CacheService::m_tid), MakeTypeIdChecker())
            .AddAttribute("window",
            "The window for measuring the average throughput (Time)",
            TimeValue(Time("10s")), MakeTimeAccessor(&CacheService::m_window), MakeTimeChecker())
            .AddTraceSource("Tx", "A new packet is created and is sent",
            MakeTraceSourceAccessor(&CacheService::m_txTrace), "ns3::Packet::TracedCallback");
        return tid;
    }

    CacheService::CacheService()
    {
        NS_LOG_FUNCTION(this);
        m_socket = 0;
        m_totalRx = 0;
    }

    CacheService::~CacheService()
    {
        NS_LOG_FUNCTION(this);
    }

    Ptr<Socket> CacheService::GetListeningSocket(void) const {
        NS_LOG_FUNCTION(this);
        return m_socket;
    }

    std::list<Ptr<Socket>> CacheService::GetAcceptedSockets(void) const
    {
        NS_LOG_FUNCTION(this);
        return m_socketList;
    }

    void CacheService::DoDispose(void)
    {
        NS_LOG_FUNCTION(this);
        m_socket = 0;
        m_socketList.clear();

        // chain up
        Application::DoDispose();
    }

    void CacheService::StartApplication(void) {
        NS_LOG_FUNCTION(this);
        // Create the socket if not already
        if (!m_socket) {
            m_socket = Socket::CreateSocket(GetNode(), m_tid);
            m_socket->Bind(m_local);
            m_socket->Listen();
            // m_socket->ShutdownSend ();
        }

        m_socket->SetRecvCallback(MakeCallback(&CacheService::HandleRead, this));

        m_socket->SetAcceptCallback(
            MakeNullCallback<bool, Ptr<Socket>, const Address &>(),
            MakeCallback(&CacheService::HandleAccept, this));
        m_socket->SetCloseCallbacks(
            MakeCallback(&CacheService::HandlePeerClose, this),
            MakeCallback(&CacheService::HandlePeerError, this));
    }

    void CacheService::StopApplication() {    // Called at time specified by Stop
        NS_LOG_FUNCTION(this);
        while (!m_socketList.empty()) {  //these are accepted sockets, close them
            Ptr<Socket> acceptedSocket = m_socketList.front();
            m_socketList.pop_front();
            acceptedSocket->Close();
        } if (m_socket) {
            m_socket->Close();
            m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> >());
        }
    }

    void CacheService::HandleRead(Ptr<Socket> socket) {
        NS_LOG_FUNCTION(this << socket);
        Ptr<Packet> packet;
        Address from;
        while ((packet = socket->RecvFrom(from))) {
            if (packet->GetSize() == 0) { //EOF
                break;
            }
            m_totalRx += packet->GetSize();

            HTTPHeader header;
            packet->RemoveHeader(header);

            SendSegment(header.GetVideoId(), header.GetResolution(),
            header.GetSegmentId(), socket);

            if (InetSocketAddress::IsMatchingType(from)) {
                NS_LOG_INFO(
                    "At time " << Simulator::Now ().GetSeconds () << "s packet sink received " << packet->GetSize () << " bytes from " << InetSocketAddress::ConvertFrom(from).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (from).GetPort () << " total Rx " << m_totalRx << " bytes");
            } else if (Inet6SocketAddress::IsMatchingType(from)) {
                NS_LOG_INFO(
                    "At time " << Simulator::Now ().GetSeconds () << "s packet sink received " << packet->GetSize () << " bytes from " << Inet6SocketAddress::ConvertFrom(from).GetIpv6 () << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort () << " total Rx " << m_totalRx << " bytes");
            }
            m_rxTrace(packet, from);
        }
    }

    void CacheService::HandlePeerClose(Ptr<Socket> socket) {
        NS_LOG_FUNCTION(this << socket);
    }

    void CacheService::HandlePeerError(Ptr<Socket> socket) {
        NS_LOG_FUNCTION(this << socket);
    }

    void CacheService::HandleAccept(Ptr<Socket> s, const Address& from) {
        NS_LOG_FUNCTION(this << s << from);

        std::cout << "[Cloude Node] Connecting Client ..." << '\n';

        s->SetRecvCallback(MakeCallback(&CacheService::HandleRead, this));
        s->SetSendCallback(MakeCallback(&CacheService::DataSend, this));

        m_socketList.push_back(s);
    }

    void  CacheService::DataSend(Ptr<Socket> socket, uint32_t) {
        NS_LOG_FUNCTION(this);
        for (std::map<Ptr<Socket>, std::queue<Packet> >::iterator iter = m_queues.begin(); iter != m_queues.end(); ++iter) {
            HTTPHeader httpHeader;
            MPEGHeader mpegHeader;

            if (iter->second.size()) {
                Ptr<Packet> frame = iter->second.front().Copy();

                frame->RemoveHeader(mpegHeader);
                frame->RemoveHeader(httpHeader);

                NS_LOG_INFO(
                "VidId: " << httpHeader.GetVideoId() << " rxAv= " << iter->first->GetRxAvailable() << " queue= "<< iter->second.size() << " res= " << httpHeader.GetResolution());
            }
        }

        while (!m_queues[socket].empty()) {
            int bytes;
            Ptr<Packet> frame = m_queues[socket].front().Copy();
            if ((bytes = socket->Send(frame)) != (int) frame->GetSize()) {
                NS_LOG_INFO("Could not send frame");
                if (bytes != -1) {
                    NS_FATAL_ERROR("Oops, we sent half a frame :(");
                }
                break;
            }
            m_queues[socket].pop();
        }

        NS_LOG_INFO("DATA WAS JUST SENT!!!");
    }

    void CacheService::SendSegment(uint32_t video_id, uint32_t resolution, uint32_t segment_id, Ptr<Socket> socket) {
        int avg_packetsize = resolution / (50 * 8);

        HTTPHeader http_header_tmp;
        MPEGHeader mpeg_header_tmp;

        Ptr<UniformRandomVariable> frame_size_gen = CreateObject<UniformRandomVariable> ();

        frame_size_gen->SetAttribute ("Min", DoubleValue (0));
        frame_size_gen->SetAttribute ("Max", DoubleValue (
            std::max(
            std::min(2 * avg_packetsize, MPEG_MAX_MESSAGE)
            - (int) (mpeg_header_tmp.GetSerializedSize()
            + http_header_tmp.GetSerializedSize()), 1)));

        for (uint32_t f_id = 0; f_id < MPEG_FRAMES_PER_SEGMENT; f_id++) {
            uint32_t frame_size = (unsigned) frame_size_gen->GetValue();

            HTTPHeader http_header;
            http_header.SetMessageType(HTTP_RESPONSE);
            http_header.SetVideoId(video_id);
            http_header.SetResolution(resolution);
            http_header.SetSegmentId(segment_id);

            MPEGHeader mpeg_header;
            mpeg_header.SetFrameId(f_id);
            mpeg_header.SetPlaybackTime(
            MilliSeconds(
            (f_id + (segment_id * MPEG_FRAMES_PER_SEGMENT))
            * MPEG_TIME_BETWEEN_FRAMES)); //50 fps
            mpeg_header.SetType('B');
            mpeg_header.SetSize(frame_size);

            Ptr<Packet> frame = Create<Packet>(frame_size);
            frame->AddHeader(http_header);
            frame->AddHeader(mpeg_header);
                NS_LOG_INFO(
                "SENDING PACKET " << f_id << " " << frame->GetSize() << " res=" << http_header.GetResolution() << " size=" << mpeg_header.GetSize() << " avg=" << avg_packetsize);

            m_queues[socket].push(*frame);
        }
        DataSend(socket, 0);
    }

    void CacheService::ConnetClientToCloud() {
        NS_LOG_FUNCTION(this);

        // Create the socket if not already
        NS_LOG_INFO("trying to create connection");
        if (!client_socket) {
            NS_LOG_INFO("Just created connection");
            client_socket = Socket::CreateSocket(GetNode(), m_tid);

            // Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
            if (client_socket->GetSocketType() != Socket::NS3_SOCK_STREAM
            && client_socket->GetSocketType() != Socket::NS3_SOCK_SEQPACKET) {
                NS_FATAL_ERROR("Using HTTP with an incompatible socket type. "
                "HTTP requires SOCK_STREAM or SOCK_SEQPACKET. "
                "In other words, use TCP instead of UDP.");
            }

            if (Inet6SocketAddress::IsMatchingType(m_peer)) {
                client_socket->Bind6();
            } else if (InetSocketAddress::IsMatchingType(m_peer)) {
                client_socket->Bind();
            }

            client_socket->Connect(m_peer);
            // m_socket->SetRecvCallback(MakeCallback(&DashClient::HandleRead, this));
            // m_socket->SetConnectCallback(
            // MakeCallback(&DashClient::ConnectionSucceeded, this),
            // MakeCallback(&DashClient::ConnectionFailed, this));
            // m_socket->SetSendCallback(MakeCallback(&DashClient::DataSend, this));
        }
        NS_LOG_INFO("Just started connection");
    }

} // namespace ns3
