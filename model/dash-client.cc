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

#include <ns3/log.h>
#include <ns3/uinteger.h>
#include <ns3/tcp-socket-factory.h>
#include <ns3/simulator.h>
#include <ns3/inet-socket-address.h>
#include <ns3/inet6-socket-address.h>
#include "http-header.h"
#include "dash-client.h"

NS_LOG_COMPONENT_DEFINE("DashClient");

namespace ns3
{

  NS_OBJECT_ENSURE_REGISTERED(DashClient);

  int DashClient::m_countObjs = 0;

  TypeId
  DashClient::GetTypeId(void)
  {
    static TypeId tid =
        TypeId("ns3::DashClient").SetParent<Application>().AddConstructor<
            DashClient>().AddAttribute("SendSize",
            "The amount of data to send each time.", UintegerValue(512),
            MakeUintegerAccessor(&DashClient::m_sendSize),
            MakeUintegerChecker<uint32_t>(1)).AddAttribute("VideoId",
            "The Id of the video that is played.", UintegerValue(0),
            MakeUintegerAccessor(&DashClient::m_videoId),
            MakeUintegerChecker<uint32_t>(1)).AddAttribute("Remote",
            "The address of the destination", AddressValue(),
            MakeAddressAccessor(&DashClient::m_peer), MakeAddressChecker()).AddAttribute(
            "MaxBytes", "The total number of bytes to send. "
                "Once these bytes are sent, "
                "no data  is sent again. The value zero means "
                "that there is no limit.", UintegerValue(0),
            MakeUintegerAccessor(&DashClient::m_maxBytes),
            MakeUintegerChecker<uint32_t>()).AddAttribute("Protocol",
            "The type of protocol to use.",
            TypeIdValue(TcpSocketFactory::GetTypeId()),
            MakeTypeIdAccessor(&DashClient::m_tid), MakeTypeIdChecker()).AddTraceSource(
            "Tx", "A new packet is created and is sent",
            MakeTraceSourceAccessor(&DashClient::m_txTrace));
    return tid;
  }

  DashClient::DashClient() :
      m_bitRate(45000), m_socket(0), m_connected(false), m_totBytes(0), m_startedReceiving(
          Seconds(0)), m_bytesRecv(0), m_lastRecv(Seconds(0)), m_sumDt(
          Seconds(0)), m_lastDt(Seconds(-1)), m_id(m_countObjs++), m_requestTime(
          "0s"), m_segment_bytes(0)
  {
    NS_LOG_FUNCTION(this);
    m_parser.SetApp(this);
  }

  DashClient::~DashClient()
  {
    NS_LOG_FUNCTION(this);
  }

  /*
   void
   DashClient::SetMaxBytes (uint32_t maxBytes)
   {
   NS_LOG_FUNCTION (this << maxBytes);
   m_maxBytes = maxBytes;
   }
   */

  Ptr<Socket>
  DashClient::GetSocket(void) const
  {
    NS_LOG_FUNCTION(this);
    return m_socket;
  }

  void
  DashClient::DoDispose(void)
  {
    NS_LOG_FUNCTION(this);

    m_socket = 0;
    // chain up
    Application::DoDispose();
  }

// Application Methods
  void
  DashClient::StartApplication(void) // Called at time specified by Start
  {
    NS_LOG_FUNCTION(this);

    // Create the socket if not already

    if (!m_socket)
      {

        m_socket = Socket::CreateSocket(GetNode(), m_tid);
        /*        m_socket = Socket::CreateSocket(GetNode(), TypeId::LookupByName ("ns3::TcpTahoe"));*/

        // Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
        if (m_socket->GetSocketType() != Socket::NS3_SOCK_STREAM
            && m_socket->GetSocketType() != Socket::NS3_SOCK_SEQPACKET)
          {
            NS_FATAL_ERROR("Using HTTP with an incompatible socket type. "
                "HTTP requires SOCK_STREAM or SOCK_SEQPACKET. "
                "In other words, use TCP instead of UDP.");
          }

        if (Inet6SocketAddress::IsMatchingType(m_peer))
          {
            m_socket->Bind6();
          }
        else if (InetSocketAddress::IsMatchingType(m_peer))
          {
            m_socket->Bind();
          }

        m_socket->Connect(m_peer);
//      m_socket->ShutdownRecv ();
        m_socket->SetRecvCallback(MakeCallback(&DashClient::HandleRead, this));
        m_socket->SetConnectCallback(
            MakeCallback(&DashClient::ConnectionSucceeded, this),
            MakeCallback(&DashClient::ConnectionFailed, this));
        m_socket->SetSendCallback(MakeCallback(&DashClient::DataSend, this));
      }
  }

  void
  DashClient::StopApplication(void) // Called at time specified by Stop
  {
    NS_LOG_FUNCTION(this);

    if (m_socket != 0)
      {
        m_socket->Close();
        m_connected = false;
      }
    else
      {
        NS_LOG_WARN("DashClient found null socket to close in StopApplication");
      }
  }

// Private helpers

  void
  DashClient::RequestSegment(uint32_t bitRate)
  {
    NS_LOG_FUNCTION(this);

    Ptr<Packet> packet = Create<Packet>(100);

    HTTPHeader httpHeader;
    httpHeader.SetSeq(1);
    httpHeader.SetMessageType(HTTP_REQUEST);
    httpHeader.SetVideoId(m_videoId);
    httpHeader.SetResolution(bitRate);
    httpHeader.SetSegmentId(m_segmentId++);
    packet->AddHeader(httpHeader);

    if ((unsigned) m_socket->Send(packet) != packet->GetSize())
      {
        NS_LOG_ERROR("Oh oh. Couldn't send packet!");
        Simulator::Stop();
      }

    m_requestTime = Simulator::Now();
    m_segment_bytes = 0;

  }

  void
  DashClient::HandleRead(Ptr<Socket> socket)
  {
    NS_LOG_FUNCTION(this << socket);

    m_parser.ReadSocket(socket);

  }

  void
  DashClient::ConnectionSucceeded(Ptr<Socket> socket)
  {
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_LOGIC("DashClient Connection succeeded");
    m_connected = true;
    RequestSegment(m_bitRate);
  }

  void
  DashClient::ConnectionFailed(Ptr<Socket> socket)
  {
    NS_LOG_FUNCTION(this << socket);NS_LOG_LOGIC(
        "DashClient, Connection Failed");
  }

  void
  DashClient::DataSend(Ptr<Socket>, uint32_t)
  {
    NS_LOG_FUNCTION(this);

    if (m_connected)
      { // Only send new data if the connection has completed

        NS_LOG_INFO("Something was sent");

        /*Simulator::ScheduleNow (&DashClient::RequestSegment, this);*/
      }
    else
      {
        NS_LOG_INFO("NOT CONNECTED!!!!");
      }
  }

  void
  DashClient::MessageReceived(Packet message)
  {
    NS_LOG_FUNCTION(this << message);

    MPEGHeader mpegHeader;
    HTTPHeader httpHeader;
    uint32_t headersize = mpegHeader.GetSerializedSize()
        + httpHeader.GetSerializedSize();

    double normBitrate = 0;
    if (m_player.m_state == MPEG_PLAYER_NOT_STARTED)
      {
        m_player.Start();
      }
    else
      {
        m_totBytes += message.GetSize();

        double instBitrate = 8 * (message.GetSize() + headersize)
            / (Simulator::Now() - m_lastRecv).GetSeconds();

/*
        if (Simulator::Now() != m_lastRecv)
          {

            m_player.AddBitRate(Simulator::Now(), instBitrate);
          }
*/

        normBitrate = 0.1 * instBitrate + 0.9 * m_bitRate;

        (void) normBitrate;

        NS_LOG_INFO(
            "------RECEIVED: " << message.GetSize() << " Bitrate = " << (8 * m_totBytes / (Simulator::Now() - m_player.m_start_time).GetSeconds()) << " instBit = " << instBitrate << " normBit = " << normBitrate);
      }
    m_player.ReceiveFrame(&message);
    m_segment_bytes += message.GetSize();

    message.RemoveHeader(mpegHeader);
    message.RemoveHeader(httpHeader);
    switch (m_player.m_state)
      {
    case MPEG_PLAYER_PLAYING:
      m_sumDt += m_player.GetRealPlayTime(mpegHeader.GetPlaybackTime());
      break;
    case MPEG_PLAYER_PAUSED:
      break;
    default:
      NS_LOG_ERROR("WRONG STATE");
      Simulator::Stop();
      }

    if (mpegHeader.GetFrameId() == MPEG_FRAMES_PER_SEGMENT - 1)
      {

        double segmentTime = (Simulator::Now() - m_requestTime).GetSeconds();

        std::cout << Simulator::Now().GetSeconds() << " bytes: "
            << m_segment_bytes << " segmentTime: " << segmentTime
            << " segmentRate: " << 8 * m_segment_bytes / segmentTime
            << std::endl;

        m_player.AddBitRate(Simulator::Now(), 8 * m_segment_bytes / segmentTime);

        m_player.LogBufferLevel();

        Time currDt = m_player.GetRealPlayTime(mpegHeader.GetPlaybackTime());
        uint32_t old = m_bitRate;
        double diff = m_lastDt >= 0 ? (currDt - m_lastDt).GetSeconds() : 0;

        Time bufferDelay;

        m_player.CalcNextSegment(m_bitRate, currDt.GetSeconds(), diff,
            m_bitRate, bufferDelay);

        if (bufferDelay == Seconds(0))
          {
            RequestSegment(m_bitRate);
          }
        else
          {
            m_player.SchduleBufferWakeup(bufferDelay, this);
          }

        std::cout << Simulator::Now().GetSeconds() << " Node: " << m_id
            << " newBitRate: " << m_bitRate << " oldBitRate: " << old
            << " estBitRate: " << m_player.GetBitRateEstimate()
            << " interTime: " << m_player.m_interruption_time.GetSeconds()
            << " T: " << currDt.GetSeconds() << " dT: "
            << (m_lastDt >= 0 ? (currDt - m_lastDt).GetSeconds() : 0)
            << " del: " << bufferDelay << std::endl;

        //if (m_lastDt >= 0) {
        NS_LOG_INFO(
            " Now: " << Simulator::Now().GetSeconds() << " Node: " << m_id << " Curr: " << currDt.GetSeconds() << " last: " << m_lastDt.GetSeconds() << " Diff: " << (currDt - m_lastDt).GetSeconds() << " Diff2: " << (currDt - m_lastDt).GetSeconds() / (Simulator::Now() - m_lastRecv).GetSeconds());
        //}
        NS_LOG_INFO(
            "==== Last frame received. Requesting segment " << m_segmentId);

        (void) old;
        NS_LOG_INFO(
            "!@#$#@!$@#\t" << Simulator::Now().GetSeconds() << " old: " << old << " new: " << m_bitRate << " t: " << currDt.GetSeconds() << " dt: " << (currDt - m_lastDt).GetSeconds());

        m_lastDt = currDt;

      }
    m_lastRecv = Simulator::Now();

  }

  void
  DashClient::GetStats()
  {
    std::cout << " InterruptionTime: "
        << m_player.m_interruption_time.GetSeconds() << " interruptions: "
        << m_player.m_interrruptions << " avgRate: "
        << (1.0 * m_player.m_totalRate) / m_player.m_framesPlayed
        << " minRate: " << m_player.m_minRate << " AvgDt: "
        << m_sumDt.GetSeconds() / m_player.m_framesPlayed << std::endl;

  }

  void
  DashClient::SetPlayerTargetTime(Time time)
  {
    m_player.m_target_dt = time;
  }

  void
  DashClient::SetProtocol(Protocol protocol)
  {
    m_player.m_protocol = protocol;
  }

} // Namespace ns3
