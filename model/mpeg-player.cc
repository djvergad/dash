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

#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/random-variable.h"
#include "http-header.h"
#include "mpeg-header.h"
#include "mpeg-player.h"
#include "dash-client.h"
#include <cmath>

NS_LOG_COMPONENT_DEFINE("MpegPlayer");
namespace ns3
{

  MpegPlayer::MpegPlayer() :
      m_state(MPEG_PLAYER_NOT_STARTED), m_interrruptions(0), m_totalRate(0), m_minRate(
          100000000), m_framesPlayed(0), m_target_dt(Seconds(7.0)), m_rateChanges(
          0), m_bitrateEstimate(0.0), m_bufferDelay(
          "0s"), m_protocol(AAASH /*FUZZY*/), m_window(Seconds(10))
  {
    NS_LOG_FUNCTION(this);
  }

  MpegPlayer::~MpegPlayer()
  {
    NS_LOG_FUNCTION(this);
  }

  int
  MpegPlayer::GetQueueSize()
  {
    return m_queue.size();
  }

  Time
  MpegPlayer::GetRealPlayTime(Time playTime)
  {
    NS_LOG_INFO(
        " Start: " << m_start_time.GetSeconds() << " Inter: " << m_interruption_time.GetSeconds() << " playtime: " << playTime.GetSeconds() << " now: " << Simulator::Now().GetSeconds() << " actual: " << (m_start_time + m_interruption_time + playTime).GetSeconds());

    return m_start_time + m_interruption_time
        + (m_state == MPEG_PLAYER_PAUSED ?
            (Simulator::Now() - m_lastpaused) : Seconds(0)) + playTime
        - Simulator::Now();
  }

  void
  MpegPlayer::CalcNextSegment(uint32_t currRate, double currDt, double diff,
      uint32_t & nextRate, Time & b_delay)
  {
    switch (m_protocol)
      {
    case FUZZY: // There is another check
    case FUZZYv2: // inside CalcFuzzy to differentiate.
    case FUZZYv3:
      CalcFuzzy(currRate, currDt, diff, nextRate, b_delay);
      break;
    case AAASH:
      CalcAAASH(currRate, currDt, diff, nextRate, b_delay);
      break;
    case OSMP:
      CalcOSMP(currRate, currDt, diff, nextRate, b_delay);
      break;
    case SVAA:
      CalcSVAA(currRate, currDt, diff, nextRate, b_delay);
      break;
    default:
      NS_FATAL_ERROR("Wrong protocol");
      }
    if (currRate != nextRate)
      {
        m_rateChanges++;
      }
  }

  void
  MpegPlayer::CalcSVAA(uint32_t currRate, double currDt, double diff,
      uint32_t & nextRate, Time & b_delay)
  {

  }

  void
  MpegPlayer::CalcOSMP(uint32_t currRate, double currDt, double diff,
      uint32_t & nextRate, Time & b_delay)
  {

  }

  void
  MpegPlayer::CalcAAASH(uint32_t currRate, double currDt, double diff,
      uint32_t & nextRate, Time & b_delay)
  {

  }

  void
  MpegPlayer::CalcFuzzy(uint32_t currRate, double currDt, double diff,
      uint32_t & nextRate, Time & b_delay)
  {

  }

  Time
  MpegPlayer::CalcSendTime(uint32_t currRate, double currDt, double diff)
  {
    return Seconds(0);
  }

  void
  MpegPlayer::AddBitRate(Time time, double bitrate)
  {
    m_bitrates[time] = bitrate;
    double sum = 0;
    int count = 0;
    for (std::map<Time, double>::iterator it = m_bitrates.begin();
        it != m_bitrates.end(); ++it)
      {
        if (it->first < (Simulator::Now() - m_window))
          {
            m_bitrates.erase(it->first);
          }
        else
          {
            sum += it->second;
            count++;
          }
      }
    m_bitrateEstimate = sum / count;
    /*std::cout << bitrate << "\t\t" << m_bitrateEstimate << "\tsum= " << sum
     << "\tcount= " << count << std::endl;*/
  }

  double
  MpegPlayer::GetBufferEstimate()
  {
    double sum = 0;
    int count = 0;
    for (std::map<Time, Time>::iterator it = m_bufferState.begin();
        it != m_bufferState.end(); ++it)
      {
        sum += it->second.GetSeconds();
        count++;
      }
    return sum / count;
  }

  void
  MpegPlayer::ReceiveFrame(Ptr<Packet> message)
  {
    NS_LOG_FUNCTION(this << message);
    NS_LOG_INFO("Received Frame " << m_state);

    Ptr<Packet> msg = message->Copy();

    m_queue.push(msg);
    if (m_state == MPEG_PLAYER_PAUSED)
      {
        NS_LOG_INFO("Play resumed");
        m_state = MPEG_PLAYER_PLAYING;
        m_interruption_time += (Simulator::Now() - m_lastpaused);
        PlayFrame();
      }
    else if (m_state == MPEG_PLAYER_NOT_STARTED)
      {
        NS_LOG_INFO("Play started");
        m_state = MPEG_PLAYER_PLAYING;
        m_start_time = Simulator::Now();
        Simulator::Schedule(Simulator::Now(), &MpegPlayer::PlayFrame, this);
      }
  }

  void
  MpegPlayer::Start(void)
  {
    NS_LOG_FUNCTION(this);
    m_state = MPEG_PLAYER_PLAYING;
    m_interruption_time = Seconds(0);

  }

  void
  MpegPlayer::PlayFrame(void)
  {
    NS_LOG_FUNCTION(this);
    if (m_state == MPEG_PLAYER_DONE)
      {
        return;
      }
    if (m_queue.empty())
      {
        NS_LOG_INFO(Simulator::Now().GetSeconds() << " No frames to play");
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

    /*std::cerr << "res= " << http_header.GetResolution() << " tot="
     << m_totalRate << " played=" << m_framesPlayed << std::endl;*/

    MPEGHeader mpegHeader;
    m_queue.back()->Copy()->RemoveHeader(mpegHeader);

    Time b_t = GetRealPlayTime(mpegHeader.GetPlaybackTime());

    if (m_bufferDelay > Time("0s") && b_t < m_bufferDelay && m_dashClient)
      {
        m_dashClient->RequestSegment();
        m_bufferDelay = Seconds(0);
        m_dashClient = NULL;
      }

    NS_LOG_INFO(
        Simulator::Now().GetSeconds() << " PLAYING FRAME: " << " VidId: " << http_header.GetVideoId() << " SegId: " << http_header.GetSegmentId() << " Res: " << http_header.GetResolution() << " FrameId: " << mpeg_header.GetFrameId() << " PlayTime: " << mpeg_header.GetPlaybackTime().GetSeconds() << " Type: " << (char) mpeg_header.GetType() << " interTime: " << m_interruption_time.GetSeconds() << " queueLength: " << m_queue.size());

    /*   std::cout << " frId: " << mpeg_header.GetFrameId()
     << " playtime: " << mpeg_header.GetPlaybackTime()
     << " target: " << (m_start_time + m_interruption_time + mpeg_header.GetPlaybackTime()).GetSeconds()
     << " now: " << Simulator::Now().GetSeconds()
     << std::endl;
     */
    Simulator::Schedule(MilliSeconds(20), &MpegPlayer::PlayFrame, this);

  }

} // namespace ns3
