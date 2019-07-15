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

    }

    CacheService::~CacheService()
    {

    }

    Ptr<Socket> CacheService::GetListeningSocket(void) const
    {
        // NS_LOG_FUNCTION(this);
        return m_socket;
    }

    std::list<Ptr<Socket>> CacheService::GetAcceptedSockets(void) const
    {
        // NS_LOG_FUNCTION(this);
        return m_socketList;
    }

    void CacheService::DoDispose(void)
    {
        // NS_LOG_FUNCTION(this);
        m_socket = 0;
        m_socketList.clear();

        // chain up
        Application::DoDispose();
    }

} // namespace ns3
