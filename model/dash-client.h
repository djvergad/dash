/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Georgia Institute of Technology
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
 * Author: George F. Riley <riley@ece.gatech.edu>
 */

#ifndef DASH_CLIENT_H
#define DASH_CLIENT_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include <queue>
#include "mpeg-header.h"

namespace ns3 {

class Address;
class Socket;
class DashClient;



/*typedef void (*func_t)(Ptr<Packet>);*/

class HTTPParser {
public:
	HTTPParser ();
    virtual ~HTTPParser ();
	void ReadSocket(Ptr<Socket> socket);
	void SetApp(DashClient *app);


private:
	uint8_t m_buffer[MPEG_MAX_MESSAGE];
	uint32_t m_bytes;
	DashClient *m_app;

};

#define MPEG_PLAYER_PAUSED 1
#define MPEG_PLAYER_PLAYING 2
#define MPEG_PLAYER_NOT_STARTED 3


class MPEGPlayer {
public:
	MPEGPlayer();
    virtual ~MPEGPlayer();
	void ReceiveFrame(Ptr<Packet> message);
	int GetQueueSize();
	void Start();
	Time GetRealPlayTime(Time playTime);
	/*uint32_t CalcSendRate(uint32_t recvRate, Time dt1);*/
	uint32_t CalcSendRate(uint32_t currRate, double currDt, double diff);

	int m_state;
	Time m_interruption_time;
	int m_interrruptions;

	Time m_start_time;
	uint32_t m_totalRate;
	uint32_t m_minRate;
	uint32_t m_framesPlayed;
	Time m_target_dt;

private:
	void PlayFrame();

	Time m_lastpaused;
	std::queue<Ptr<Packet> > m_queue;

};


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
  static TypeId GetTypeId (void);

  DashClient ();

  virtual ~DashClient ();

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
  Ptr<Socket> GetSocket (void) const;

  void MessageReceived(Packet message);
  void GetStats();
  void SetPlayerTargetTime(Time time);

  MPEGPlayer	  m_player;


protected:
  virtual void DoDispose (void);
private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  void RequestSegment (uint32_t bitRate);

  Ptr<Socket>     m_socket;       // Associated socket
  Address         m_peer;         // Peer address
  bool            m_connected;    // True if connected
  uint32_t        m_sendSize;     // Size of data to send each time
  uint32_t        m_maxBytes;     // Limit total number of bytes sent
  uint32_t        m_totBytes;     // Total bytes sent so far
  TypeId          m_tid;
  TracedCallback<Ptr<const Packet> > m_txTrace;
  HTTPParser	  m_parser;
  uint32_t        m_videoId;
  uint32_t        m_segmentId;
  uint32_t        m_bitRate;
  Time            m_startedReceiving;
  uint32_t        m_bytesRecv;
  Time            m_lastRecv;
  Time 			  m_sumDt;
  Time            m_lastDt;


private:
  void ConnectionSucceeded (Ptr<Socket> socket);
  void ConnectionFailed (Ptr<Socket> socket);
  void DataSend (Ptr<Socket>, uint32_t); // for socket's SetSendCallback
  void Ignore (Ptr<Socket> socket);
  void HandleRead(Ptr<Socket>);

  static int m_countObjs;
  int m_id;
};

} // namespace ns3

#endif /* DASH_CLIENT_H */
