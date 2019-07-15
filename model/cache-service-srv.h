

#ifndef CACHE_SERVICE_SRV_H
#define CACHE_SERVICE_SRV_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include <map>
#include <queue>


namespace ns3
{

    class Address;
    class Socket;
    class Packet;

    class CacheService : public Application
    {
        public:
            static TypeId
            GetTypeId(void);

            CacheService();
            ~CacheService();

            /**
             * \return pointer to listening socket
             */
            Ptr<Socket> GetListeningSocket(void) const;

            /**
             * \return list of pointers to accepted sockets
             */
            std::list<Ptr<Socket>> GetAcceptedSockets(void) const;

        protected:
            virtual void DoDispose(void);

        private:
            // In the case of TCP, each socket accept returns a new socket, so the
            // listening socket is stored seperately from the accepted sockets
            Ptr<Socket> m_socket;       // Listening socket
            std::list<Ptr<Socket> > m_socketList; //the accepted sockets

            Address m_local;        // Local address to bind to
            uint32_t m_totalRx;      // Total bytes received
            TypeId m_tid;          // Protocol TypeId
            TracedCallback<Ptr<const Packet>, const Address &> m_rxTrace;

            // A structure that contains the generated MPEG frames, for each client.
            std::map<Ptr<Socket>, std::queue<Packet>> m_queues;
    };

}// namespace ns3

#endif /* CACHE_SERVICE_SRV_H */
