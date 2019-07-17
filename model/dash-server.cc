
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 TEI of Western Macedonia, Greece
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Dimitrios J. Vergados <djvergad@gmail.com>
 */

#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "dash-server.h"
#include "http-header.h"
#include "mpeg-header.h"
#include <ns3/random-variable-stream.h>
#include <ns3/tcp-socket-factory.h>
#include <ns3/tcp-socket.h>
#include <ns3/double.h>

namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("DashServer");
    NS_OBJECT_ENSURE_REGISTERED(DashServer);

    TypeId DashServer::GetTypeId(void) {
        static TypeId tid =
        TypeId("ns3::DashServer").SetParent<Application>().AddConstructor<DashServer>()
            .AddAttribute("Local",
            "The Address on which to Bind the rx socket.", AddressValue(),
            MakeAddressAccessor(&DashServer::m_local), MakeAddressChecker()).AddAttribute(
            "Protocol", "The type id of the protocol to use for the rx socket.",
            TypeIdValue(TcpSocketFactory::GetTypeId()),
            MakeTypeIdAccessor(&DashServer::m_tid), MakeTypeIdChecker()).AddTraceSource(
            "Rx", "A packet has been received",
            MakeTraceSourceAccessor(&DashServer::m_rxTrace), "ns3::Packet::TracedCallback");
        return tid;
    }

    DashServer::DashServer() {
        NS_LOG_FUNCTION(this);
        m_socket = 0;
        m_totalRx = 0;
    }

    DashServer::~DashServer() {
        NS_LOG_FUNCTION(this);
    }

    Ptr<Socket> DashServer::GetListeningSocket(void) const {
        NS_LOG_FUNCTION(this);
        return m_socket;
    }

    std::list<Ptr<Socket> > DashServer::GetAcceptedSockets(void) const {
        NS_LOG_FUNCTION(this);
        return m_socketList;
    }

    void DashServer::DoDispose(void) {
        NS_LOG_FUNCTION(this);
        m_socket = 0;
        m_socketList.clear();

        // chain up
        Application::DoDispose();
    }

    // Application Methods
    void DashServer::StartApplication() {    // Called at time specified by Start
        NS_LOG_FUNCTION(this);
        // Create the socket if not already
        if (!m_socket) {
            m_socket = Socket::CreateSocket(GetNode(), m_tid);
            m_socket->Bind(m_local);
            m_socket->Listen();
            // m_socket->ShutdownSend ();
            if (addressUtils::IsMulticast(m_local)) {
                Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket>(m_socket);
                if (udpSocket) {
                    // equivalent to setsockopt (MCAST_JOIN_GROUP)
                    udpSocket->MulticastJoinGroup(0, m_local);
                } else {
                    NS_FATAL_ERROR("Error: joining multicast on a non-UDP socket");
                }
            }
        }

        m_socket->SetRecvCallback(MakeCallback(&DashServer::HandleRead, this));

        m_socket->SetAcceptCallback(
            MakeNullCallback<bool, Ptr<Socket>, const Address &>(),
            MakeCallback(&DashServer::HandleAccept, this));
        m_socket->SetCloseCallbacks(
            MakeCallback(&DashServer::HandlePeerClose, this),
            MakeCallback(&DashServer::HandlePeerError, this));
    }

    void DashServer::StopApplication() {    // Called at time specified by Stop
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

    void DashServer::HandleRead(Ptr<Socket> socket) {
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

    void DashServer::HandlePeerClose(Ptr<Socket> socket) {
        NS_LOG_FUNCTION(this << socket);
    }

    void DashServer::HandlePeerError(Ptr<Socket> socket) {
        NS_LOG_FUNCTION(this << socket);
    }

    void DashServer::HandleAccept(Ptr<Socket> s, const Address& from) {
        NS_LOG_FUNCTION(this << s << from);

        std::cout << "[Cloude Node] Connecting Client ..." << '\n';

        s->SetRecvCallback(MakeCallback(&DashServer::HandleRead, this));
        s->SetSendCallback(MakeCallback(&DashServer::DataSend, this));

        m_socketList.push_back(s);

        this->nodeMap[s] = NodeType::User;

        Ptr<Socket> fog = ConnectFog();

        initVideoStream();

        // this->streams[fog.getSocketId()].fogsocket = fog;
        // this->nodeMap[fog.getSocketId()] = NodeType::Fog;
        // cout << "VIDEO STREAM CREATED | USER=" << connId << ", FOG=" << fog.getSocketId() << endl;
    }

    Ptr<Socket> DashServer::ConnectFog(void) {
        Ptr<Socket> fognode;

        NS_LOG_INFO("Just created fog connection");

        std::cout << "[Cloude Node] Just created fog connection" << '\n';

        TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");

        fognode = Socket::CreateSocket(GetNode(), tid);

        // Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
        if (fognode->GetSocketType() != Socket::NS3_SOCK_STREAM
            && fognode->GetSocketType() != Socket::NS3_SOCK_SEQPACKET) {
            NS_FATAL_ERROR("Using HTTP with an incompatible socket type. "
            "HTTP requires SOCK_STREAM or SOCK_SEQPACKET. "
            "In other words, use TCP instead of UDP.");
        }

        if (Inet6SocketAddress::IsMatchingType(f_peer)) {
            fognode->Bind6();
        } else if (InetSocketAddress::IsMatchingType(f_peer)) {
            fognode->Bind();
        }

        fognode->Connect(f_peer);

        // fognode->SetRecvCallback(MakeCallback(&DashClient::HandleRead, this));
        // fognode->SetConnectCallback(
        //     MakeCallback(&DashClient::ConnectionSucceeded, this),
        //     MakeCallback(&DashClient::ConnectionFailed, this));
        // fognode->SetSendCallback(MakeCallback(&DashClient::DataSend, this));

        return fognode;
    }

    void DashServer::initVideoStream(void) {
        VideoStreamDash vsm;

        f_connected = true;
        f_packetSent = 0;
        if(InetSocketAddress::IsMatchingType(f_peer)){
            f_socket->Bind();
        } else {
            f_socket->Bind6();
        }
        f_socket->Connect();
        SendPacket();
    }

    void DashServer::SendPacket(void) {
        Ptr<Packet> packet = Create<Packet>(f_packetSize);

        f_socket->Send(packet);


    }

    void  DashServer::DataSend(Ptr<Socket> socket, uint32_t) {
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

    void DashServer::SendSegment(uint32_t video_id, uint32_t resolution, uint32_t segment_id, Ptr<Socket> socket) {
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

} // Namespace ns3
