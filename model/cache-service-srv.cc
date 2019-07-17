#include "cache-service-srv.h"

#include "ns3/log.h"
#include "ns3/udp-socket.h"
#include "ns3/socket-factory.h"
#include "ns3/udp-socket-factory.h"

#include <ns3/random-variable-stream.h>
#include <ns3/tcp-socket.h>
#include <ns3/double.h>


namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("CacheService");
    NS_OBJECT_ENSURE_REGISTERED(CacheService);

    TypeId CacheService::GetTypeId(void)
    {
        static TypeId tid =
          TypeId("ns3::CacheService").SetParent<Application>().AddConstructor<CacheService>().AddAttribute("Local",
              "The Address on which to Bind the rx socket.", AddressValue(),
              MakeAddressAccessor(&CacheService::m_local), MakeAddressChecker()).AddAttribute(
              "Protocol", "The type id of the protocol to use for the rx socket.",
              TypeIdValue(UdpSocketFactory::GetTypeId()),
              MakeTypeIdAccessor(&CacheService::m_tid), MakeTypeIdChecker()).AddTraceSource(
              "Rx", "A packet has been received",
              MakeTraceSourceAccessor(&CacheService::m_rxTrace), "ns3::Packet::TracedCallback");
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

    }

    void CacheService::HandleRead(Ptr<Socket> socket) {

    }

    void CacheService::HandlePeerClose(Ptr<Socket> socket) {
        NS_LOG_FUNCTION(this << socket);
    }

    void CacheService::HandlePeerError(Ptr<Socket> socket) {
        NS_LOG_FUNCTION(this << socket);
    }

    void CacheService::HandleAccept(Ptr<Socket> s, const Address& from) {
    }

    void  CacheService::DataSend(Ptr<Socket> socket, uint32_t) {
    }



} // namespace ns3
