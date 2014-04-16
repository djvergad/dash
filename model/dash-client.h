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

#ifndef DASH_CLIENT_H
#define DASH_CLIENT_H

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"
#include "mpeg-player.h"
#include "ns3/traced-callback.h"
#include "http-parser.h"

namespace ns3
{

  /**
   * \ingroup applications
   * \defgroup http DashClient
   *
   * This traffic generator simply sends data
   * as fast as possible up to MaxBytes or until
   * the appplication is stopped if MaxBytes is
   * zero. Once the lower layer send buffer is
   * filled, it waits until space is free to
   * send more data, essentially keeping a
   * constant flow of data. Only SOCK_STREAM
   * and SOCK_SEQPACKET sockets are supported.
   * For example, TCP sockets can be used, but
   * UDP sockets can not be used.
   */
  class DashClient : public Application
  {
  public:
    static TypeId
    GetTypeId(void);

    DashClient();

    virtual
    ~DashClient();

    /*  *
     * \param maxBytes the upper bound of bytes to send
     *
     * Set the upper bound for the total number of bytes to send. Once
     * this bound is reached, no more application bytes are sent. If the
     * application is stopped during the simulation and restarted, the
     * total number of bytes sent is not reset; however, the maxBytes
     * bound is still effective and the application will continue sending
     * up to maxBytes. The value zero for maxBytes means that
     * there is no upper bound; i.e. data is sent until the application
     * or simulation is stopped.

     void SetMaxBytes (uint32_t maxBytes);*/

    /**
     * \return pointer to associated socket
     */
    Ptr<Socket>
    GetSocket(void) const;

    void
    MessageReceived(Packet message);
    void
    GetStats();
    void
    SetPlayerTargetTime(Time time);

    MpegPlayer m_player;

    void
    RequestSegment(uint32_t bitRate);
    uint32_t m_bitRate;

    inline MpegPlayer &
    GetPlayer()
    {
      return m_player;
    }

  protected:
    virtual void
    DoDispose(void);

  private:
    // inherited from Application base class.
    virtual void
    StartApplication(void);    // Called at time specified by Start
    virtual void
    StopApplication(void);     // Called at time specified by Stop

    Ptr<Socket> m_socket;       // Associated socket
    Address m_peer;         // Peer address
    bool m_connected;    // True if connected
    uint32_t m_sendSize;     // Size of data to send each time
    uint32_t m_maxBytes;     // Limit total number of bytes sent
    uint32_t m_totBytes;     // Total bytes sent so far
    TypeId m_tid;
    TracedCallback<Ptr<const Packet> > m_txTrace;
    HttpParser m_parser;
    uint32_t m_videoId;
    uint32_t m_segmentId;
    Time m_startedReceiving;
    uint32_t m_bytesRecv;
    Time m_lastRecv;
    Time m_sumDt;
    Time m_lastDt;

  private:
    void
    ConnectionSucceeded(Ptr<Socket> socket);
    void
    ConnectionFailed(Ptr<Socket> socket);
    void
    DataSend(Ptr<Socket>, uint32_t); // for socket's SetSendCallback
    void
    Ignore(Ptr<Socket> socket);
    void
    HandleRead(Ptr<Socket>);

    static int m_countObjs;
    int m_id;
    Time m_requestTime;
    uint32_t m_segment_bytes;
  };

} // namespace ns3

#endif /* DASH_CLIENT_H */
