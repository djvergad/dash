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

#ifndef DASH_SERVER_H
#define DASH_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/socket.h"

#include <ns3/data-rate.h>

// #include "video-stream-dash.h"

#include <map>
#include <queue>


namespace ns3
{

    class Address;
    class Socket;
    class Packet;

    /**
    * \ingroup dash
    *
    * \breif This application was written to complement DashClient. It received
    * requests for MPEG Segments from clients, and responds by transmitting back
    * the MPEG frames that are contained in the frame
    *
    * The constructor specifies the Address (IP  address and port) and the
    * transport protocol to use.   A virtual Receive () method is installed
    * as a callback on the receiving socket.  By default, when logging is
    * enabled, it prints out the size of packets and their address, but
    * we intend to also add a tracing source to Receive() at a later date.
    */



    enum NodeType {
        User, Fog, Hybrid
    };

    typedef struct VideoStreamDash_h
    {
        int clientIndex;
        int fogIndex;
        Ptr<Socket> client;
        Ptr<Socket> fognode;
    } VideoStreamDash;

    class DashServer : public Application
    {
        public:
            static TypeId GetTypeId(void);
            DashServer();

            virtual ~DashServer();

            /**
            * \return pointer to listening socket
            */
            Ptr<Socket> GetListeningSocket(void) const;

            /**
            * \return list of pointers to accepted sockets
            */
            std::list<Ptr<Socket> > GetAcceptedSockets(void) const;

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

            Ptr<Socket> ConnectFog(void);

            void initVideoStream(void);

            // In the case of TCP, each socket accept returns a new socket, so the
            // listening socket is stored seperately from the accepted sockets
            Ptr<Socket> m_socket;       // Listening socket
            std::list<Ptr<Socket> > m_socketList; //the accepted sockets

            TypeId m_tid;          // Protocol TypeId
            Address m_local;        // Local address to bind to
            uint32_t m_totalRx;      // Total bytes received
            TracedCallback<Ptr<const Packet>, const Address &> m_rxTrace;


            // Fog Socket Methods
            void Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate DataRate);

            void SendPacket(void);
            void ScheduleTx(void);

            Ptr<Socket> f_socket;
            Address     f_peer;          // Peer address
            uint32_t    f_packetSize;
            uint32_t    f_nPackets;
            DataRate    f_dataRate;
            EventId     f_sendEvent;
            bool        f_connected;
            uint32_t    f_packetSent;

            // A structure that contains the generated MPEG frames, for each client.
            std::map<Ptr<Socket>, std::queue<Packet> > m_queues;

            std::map<Ptr<Socket>, NodeType> nodeMap;
    };

} // namespace ns3

#endif /* DASH_SERVER_H */
