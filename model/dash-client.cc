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

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include "dash-client.h"
#include "http-header.h"
#include "mpeg-header.h"
#include <string.h>
#include <cmath>
#include <ns3/inet6-socket-address.h>
#include <ns3/inet-socket-address.h>
#include "ns3/random-variable.h"

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
      m_socket(0), m_connected(false), m_totBytes(0), m_bitRate(13281), m_startedReceiving(
          Seconds(0)), m_bytesRecv(0), m_lastRecv(Seconds(0)), m_sumDt(
          Seconds(0)), m_lastDt(Seconds(-1)), m_id(m_countObjs++)
  {
    NS_LOG_FUNCTION (this);
    m_parser.SetApp(this);
  }

  DashClient::~DashClient()
  {
    NS_LOG_FUNCTION (this);
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
    NS_LOG_FUNCTION (this);
    return m_socket;
  }

  void
  DashClient::DoDispose(void)
  {
    NS_LOG_FUNCTION (this);

    m_socket = 0;
    // chain up
    Application::DoDispose();
  }

// Application Methods
  void
  DashClient::StartApplication(void) // Called at time specified by Start
  {
    NS_LOG_FUNCTION (this);

    // Create the socket if not already
    if (!m_socket)
      {
        m_socket = Socket::CreateSocket(GetNode(), m_tid);

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
    NS_LOG_FUNCTION (this);

    if (m_socket != 0)
      {
        m_socket->Close();
        m_connected = false;
      }
    else
      {
        NS_LOG_WARN ("DashClient found null socket to close in StopApplication");
      }
  }

// Private helpers

  void
  DashClient::RequestSegment(uint32_t bitRate)
  {
    NS_LOG_FUNCTION (this);

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

  }

  void
  DashClient::HandleRead(Ptr<Socket> socket)
  {
    NS_LOG_FUNCTION (this << socket);

    m_parser.ReadSocket(socket);

  }

  void
  DashClient::ConnectionSucceeded(Ptr<Socket> socket)
  {
    NS_LOG_FUNCTION (this << socket);NS_LOG_LOGIC ("DashClient Connection succeeded");
    m_connected = true;
    RequestSegment(100000);
  }

  void
  DashClient::ConnectionFailed(Ptr<Socket> socket)
  {
    NS_LOG_FUNCTION (this << socket);NS_LOG_LOGIC ("DashClient, Connection Failed");
  }

  void
  DashClient::DataSend(Ptr<Socket>, uint32_t)
  {
    NS_LOG_FUNCTION (this);

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
    NS_LOG_FUNCTION (this << message);

    double normBitrate = 0;
    if (m_player.m_state == MPEG_PLAYER_NOT_STARTED)
      {
        m_player.Start();
      }
    else
      {
        m_totBytes += message.GetSize();

        double instBitrate = 8 * message.GetSize()
            / (Simulator::Now() - m_lastRecv).GetSeconds();
        normBitrate = 0.1 * instBitrate + 0.9 * m_bitRate;

        (void) normBitrate;

        NS_LOG_INFO("------RECEIVED: " << message.GetSize()
            << " Bitrate = " << (8 * m_totBytes / (Simulator::Now() - m_player.m_start_time).GetSeconds())
            << " instBit = " << instBitrate
            << " normBit = " << normBitrate);
        /*
         (void) bitrate;
         (void) instBitrate;
         (void) normBitrate;*/
        /*if (false) {
         std::cout << bitrate << instBitrate << normBitrate << std::endl;
         }*/

        //	  m_bitRate = normBitrate;
      }
    m_player.ReceiveFrame(&message);

    MPEGHeader mpegHeader;
    message.RemoveHeader(mpegHeader);
    HTTPHeader httpHeader;
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
        Time currDt = m_player.GetRealPlayTime(mpegHeader.GetPlaybackTime());
        double old = m_bitRate;
        m_bitRate = m_player.CalcSendRate(m_bitRate, currDt.GetSeconds(),
            m_lastDt >= 0 ? (currDt - m_lastDt).GetSeconds() : 0);
        RequestSegment(m_bitRate);

        std::cout << Simulator::Now().GetSeconds() << " Node: " << m_id
            << " newBitRate: " << m_bitRate << " oldBitRate: " << old
            << " interTime: " << m_player.m_interruption_time.GetSeconds()
            << " T: " << currDt.GetSeconds() << " dT: "
            << (m_lastDt >= 0 ? (currDt - m_lastDt).GetSeconds() : 0)
            << std::endl;

        //if (m_lastDt >= 0) {
        NS_LOG_INFO(" Now: " << Simulator::Now().GetSeconds()
            << " Node: " << m_id
            << " Curr: " << currDt.GetSeconds()
            << " last: " << m_lastDt.GetSeconds()
            << " Diff: " << (currDt - m_lastDt).GetSeconds()
            << " Diff2: " << (currDt - m_lastDt).GetSeconds()
            / (Simulator::Now() - m_lastRecv).GetSeconds()
        );
        //}
        NS_LOG_INFO("==== Last frame received. Requesting segment " << m_segmentId);

        (void) old;
        NS_LOG_INFO("!@#$#@!$@#\t" << Simulator::Now().GetSeconds()
            << " old: " << old
            << " new: " << m_bitRate
            << " t: " << currDt.GetSeconds()
            << " dt: " << (currDt - m_lastDt).GetSeconds()
        );

        m_lastRecv = Simulator::Now();
        m_lastDt = currDt;

      }
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

  HTTPParser::HTTPParser() :
      m_bytes(0), m_app(NULL)
  {
    NS_LOG_FUNCTION (this);
  }

  HTTPParser::~HTTPParser()
  {
    NS_LOG_FUNCTION (this);
  }

  void
  HTTPParser::SetApp(DashClient *app)
  {
    NS_LOG_FUNCTION (this << app);
    m_app = app;
  }
  void
  HTTPParser::ReadSocket(Ptr<Socket> socket)
  {
    NS_LOG_FUNCTION (this << socket);
    Address from;
    int bytes = socket->RecvFrom(&m_buffer[m_bytes], MPEG_MAX_MESSAGE - m_bytes,
        0, from);
    if (bytes > 0)
      {
        m_bytes += bytes;
      }

    NS_LOG_INFO("### Buffer space: " << m_bytes << " Queue length " << m_app->m_player.GetQueueSize());

    MPEGHeader mpeg_header;
    HTTPHeader http_header;

    uint32_t headersize = mpeg_header.GetSerializedSize()
        + http_header.GetSerializedSize();

    if (m_bytes < headersize)
      {
        return;
      }

    Packet headerPacket(m_buffer, headersize);
    headerPacket.RemoveHeader(mpeg_header);

    uint32_t message_size = headersize + mpeg_header.GetSize();

    if (m_bytes < message_size)
      {
        return;
      }
    Packet message(m_buffer, message_size);

    memmove(m_buffer, &m_buffer[message_size], m_bytes - message_size);
    m_bytes -= message_size;

    m_app->MessageReceived(message);

    ReadSocket(socket);
  }

  MPEGPlayer::MPEGPlayer() :
      m_state(MPEG_PLAYER_NOT_STARTED), m_interrruptions(0), m_totalRate(0), m_minRate(
          100000000), m_target_dt(Seconds(7.0))
  {
    NS_LOG_FUNCTION (this);
  }

  MPEGPlayer::~MPEGPlayer()
  {
    NS_LOG_FUNCTION (this);
  }

  int
  MPEGPlayer::GetQueueSize()
  {
    return m_queue.size();
  }

  Time
  MPEGPlayer::GetRealPlayTime(Time playTime)
  {
    NS_LOG_INFO(" Start: " << m_start_time.GetSeconds()
        << " Inter: " << m_interruption_time.GetSeconds()
        << " playtime: " << playTime.GetSeconds()
        << " now: " << Simulator::Now().GetSeconds()
        << " actual: " << (m_start_time + m_interruption_time + playTime).GetSeconds());

    return m_start_time + m_interruption_time
        + (m_state == MPEG_PLAYER_PAUSED ?
            (Simulator::Now() - m_lastpaused) : Seconds(0)) + playTime
        - Simulator::Now();
  }

  /*uint32_t
   MPEGPlayer::CalcSendRate(uint32_t recvRate, Time dt1)
   {
   Time target = m_target_dt;

   if (dt1 > 2 * m_target_dt) {
   target =  Seconds(dt1.GetSeconds() / 2);
   }

   double factor = (1-(target - dt1).GetSeconds()/(MPEG_FRAMES_PER_SEGMENT * 0.020));

   if (factor > 1) {
   factor /= 2;
   }

   if (factor < 0) {
   factor = 0.0001;
   }

   return factor * recvRate / 2;

   std::cout << recvRate << " " << dt1.GetSeconds() << " " << factor << " " << factor * recvRate << std::endl;
   return factor * recvRate > 500000? 500000 : factor *recvRate  ;

   }*/

  uint32_t
  MPEGPlayer::CalcSendRate(uint32_t currRate, double currDt, double diff)
  {
    double slow = 0, ok = 0, fast = 0, falling = 0, steady = 0, rising = 0, r1 =
        0, r2 = 0, r3 = 0, r4 = 0, r5 = 0, r6 = 0, r7 = 0, r8 = 0, r9 = 0, p2 =
        0, p1 = 0, z = 0, n1 = 0, n2 = 0, output = 0;

    double t = m_target_dt.GetSeconds();
    if (currDt < 2 * t / 3)
      {
        slow = 1.0;
      }
    else if (currDt < t)
      {
        slow = 1 - 1 / (t / 3) * (currDt - 2 * t / 3);
        ok = 1 / (t / 3) * (currDt - 2 * t / 3);
      }
    else if (currDt < 4 * t)
      {
        ok = 1 - 1 / (3 * t) * (currDt - t);
        fast = 1 / (3 * t) * (currDt - t);
      }
    else
      {
        fast = 1;
      }

    if (diff < -2 * t / 3)
      {
        falling = 1;
      }
    else if (diff < 0)
      {
        falling = 1 - 1 / (2 * t / 3) * (diff + 2 * t / 3);
        steady = 1 / (2 * t / 3) * (diff + 2 * t / 3);
      }
    else if (diff < 4 * t)
      {
        steady = 1 - 1 / (4 * t) * diff;
        rising = 1 / (4 * t) * diff;
      }
    else
      {
        rising = 1;
      }

    r1 = std::min(slow, falling);
    r2 = std::min(ok, falling);
    r3 = std::min(fast, falling);
    r4 = std::min(slow, steady);
    r5 = std::min(ok, steady);
    r6 = std::min(fast, steady);
    r7 = std::min(slow, rising);
    r8 = std::min(ok, rising);
    r9 = std::min(fast, rising);

    p2 = std::sqrt(std::pow(r9, 2));
    p1 = std::sqrt(std::pow(r6, 2) + std::pow(r8, 2));
    z = std::sqrt(std::pow(r3, 2) + std::pow(r5, 2) + std::pow(r7, 2));
    n1 = std::sqrt(std::pow(r2, 2) + std::pow(r4, 2));
    n2 = std::sqrt(std::pow(r1, 2));

    output = (n2 * 0.25 + n1 * 0.5 + z * 1 + p1 * 1.4 + p2 * 2)
        / (n2 + n1 + z + p1 + p2);

    NS_LOG_INFO("currDt: " << currDt
        << " diff: " << diff
        << " slow: " << slow
        << " ok: " << ok
        << " fast: " << fast
        << " falling: " << falling
        << " steady: " << steady
        << " rising: " << rising
        << " r1: " << r1
        << " r2: " << r2
        << " r3: " << r3
        << " r4: " << r4
        << " r5: " << r5
        << " r6: " << r6
        << " r7: " << r7
        << " r8: " << r8
        << " r9: " << r9
        << " p2: " << p2
        << " p1: " << p1
        << " z: " << z
        << " n1: " << n1
        << " n2: " << n2
        << " output: " << output
    );

    uint32_t result = output * currRate;

    uint32_t nextRate = result > 537823 ? 537823
        : result > 384159 ? 384159
        : result > 274399 ? 274399
        : result > 195999 ? 195999
        : result > 140000 ? 140000
        : result > 100000 ? 100000
        : result > 71428 ? 71428
        : result > 51020 ? 51020
        : result > 36443 ? 36443
        : result > 26030 ? 26030
        : result > 18593 ? 18593
        : 13281;

    if (nextRate > currRate)
      {
        double incrProb = std::pow(0.8, (std::log10((double)currRate) -5)/ std::log10(1.4));

        UniformVariable uv;
        double rand = uv.GetValue();
        NS_LOG_INFO(currRate << " " << incrProb << " " << rand);
        if (rand  > incrProb)
          {
            nextRate = currRate;
          }

      }

    NS_LOG_INFO( currRate << " " << output << " " << result);

    /*result = result > 100000 ? result : 100000;
     result = result < 400000 ? result : 400000;*/

    return nextRate;
  }

  void
  MPEGPlayer::ReceiveFrame(Ptr<Packet> message)
  {
    NS_LOG_FUNCTION (this << message);

    Ptr<Packet> msg = message->Copy();

    m_queue.push(msg);
    if (m_state == MPEG_PLAYER_PAUSED)
      {
        m_state = MPEG_PLAYER_PLAYING;
        m_interruption_time += (Simulator::Now() - m_lastpaused);
        PlayFrame();
      }

  }

  void
  MPEGPlayer::Start(void)
  {
    NS_LOG_FUNCTION (this);
    m_state = MPEG_PLAYER_PLAYING;
    m_interruption_time = Seconds(0);
    m_start_time = Simulator::Now() + m_target_dt;
    Simulator::Schedule(m_target_dt, &MPEGPlayer::PlayFrame, this);
  }

  void
  MPEGPlayer::PlayFrame(void)
  {
    NS_LOG_FUNCTION (this);
    if (m_queue.empty())
      {
        NS_LOG_INFO("No frames to play");
        m_state = MPEG_PLAYER_PAUSED;
        m_lastpaused = Simulator::Now();
        m_interrruptions++;
        return;
      }
    Ptr<Packet> message = m_queue.front();
    m_queue.pop();

    MPEGHeader mpeg_header;
    HTTPHeader http_header;

    message->RemoveHeader(mpeg_header);
    message->RemoveHeader(http_header);

    m_totalRate += http_header.GetResolution();
    m_minRate =
        http_header.GetResolution() < m_minRate ?
            http_header.GetResolution() : m_minRate;
    m_framesPlayed++;

    NS_LOG_INFO(Simulator::Now().GetSeconds() << " PLAYING FRAME: "
        << " VidId: " << http_header.GetVideoId()
        << " SegId: " << http_header.GetSegmentId()
        << " Res: " << http_header.GetResolution()
        << " FrameId: " << mpeg_header.GetFrameId()
        << " PlayTime: " << mpeg_header.GetPlaybackTime().GetSeconds()
        << " Type: " <<(char) mpeg_header.GetType()
        << " interTime: " << m_interruption_time.GetSeconds()
        << " queueLength: " << m_queue.size());

    /*   std::cout << " frId: " << mpeg_header.GetFrameId()
     << " playtime: " << mpeg_header.GetPlaybackTime()
     << " target: " << (m_start_time + m_interruption_time + mpeg_header.GetPlaybackTime()).GetSeconds()
     << " now: " << Simulator::Now().GetSeconds()
     << std::endl;
     */
    Simulator::Schedule(MilliSeconds(20), &MPEGPlayer::PlayFrame, this);

  }

} // Namespace ns3
