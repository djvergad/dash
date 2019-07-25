

#ifndef CACHE_SERVICE_SRV_H
#define CACHE_SERVICE_SRV_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"

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

            void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

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
            // inherited from Application base class.
            virtual void StartApplication(void);    // Called at time specified by Start
            virtual void StopApplication(void);     // Called at time specified by Stop

            void HandleRead(Ptr<Socket>);   // Called when a request is received
            void DataSend(Ptr<Socket>, uint32_t); // Called when a new segment is transmitted
                                     // or when new space is aveilable in the buffer
            void SendSegment(uint32_t video_id, uint32_t resolution, uint32_t segment_id, Ptr<Socket> socket);  // Sends the segment back to the client

            void HandleAccept(Ptr<Socket>, const Address& from); // Called hen a new connection is accepted
            void HandlePeerClose(Ptr<Socket>); // Called when the connection is closed by the peer.
            void HandlePeerError(Ptr<Socket>); // Called when there is a peer error

            void ConnetClientToCloud();

            // In the case of TCP, each socket accept returns a new socket, so the
            // listening socket is stored seperately from the accepted sockets
            Ptr<Socket> m_socket;    // Listening socket
            std::list<Ptr<Socket> > m_socketList; //the accepted sockets

            Ptr<Socket> client_socket;  // Associated Socket

            Address m_peer;        // Peer address

            Address m_local;       // Local address to bind to


            uint32_t m_fog_segmentId;    // The id of the current segment

            uint32_t m_totalRx;    // Total bytes received
            TypeId m_tid;          // Protocol TypeId
            TracedCallback<Ptr<const Packet>, const Address &> m_rxTrace;
            TracedCallback<Ptr<const Packet> > m_txTrace;

            // A structure that contains the generated MPEG frames, for each client.
            std::map<Ptr<Socket>, std::queue<Packet>> m_queues;

            Time m_window;

    };

}// namespace ns3

#endif /* CACHE_SERVICE_SRV_H */
