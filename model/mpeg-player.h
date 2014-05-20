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

#ifndef MPEG_PLAYER_H_
#define MPEG_PLAYER_H_

#include <queue>
#include <map>
#include "ns3/ptr.h"
#include "ns3/packet.h"

namespace ns3
{
  enum
  {
    MPEG_PLAYER_PAUSED, MPEG_PLAYER_PLAYING, MPEG_PLAYER_NOT_STARTED
  };

  enum Protocol
  {
    FUZZY, AAASH, FUZZYv2, FUZZYv3, FUZZYv4, OSMP, SVAA
  };

  class DashClient;

  class MpegPlayer
  {
  public:
    MpegPlayer();

    virtual
    ~MpegPlayer();

    void
    ReceiveFrame(Ptr<Packet> message);

    int
    GetQueueSize();

    void
    Start();

    Time
    GetRealPlayTime(Time playTime);

    void
    CalcNextSegment(uint32_t currRate, double currDt, double diff,
        uint32_t & nextRate, Time & b_delay);

    Time
    CalcSendTime(uint32_t currRate, double currDt, double diff);

    void
    AddBitRate(Time time, double bitrate);

    void
    LogBufferLevel(Time t);

    double inline
    GetBitRateEstimate()
    {
      return m_bitrateEstimate;
    }

    double
    GetBufferEstimate();

    void inline
    SchduleBufferWakeup(const Time t, DashClient * client)
    {
      m_bufferDelay = t;
      m_dashClient = client;

    }

    void inline
    SetProtocol(Protocol protocol)
    {
      m_protocol = protocol;

    }

    void inline
    SetWindow(Time time)
    {
      m_window = time;
    }

    int m_state;
    Time m_interruption_time;
    int m_interrruptions;

    Time m_start_time;
    uint32_t m_totalRate;
    uint32_t m_minRate;
    uint32_t m_framesPlayed;
    Time m_target_dt;

  private:
    void
    PlayFrame();

    void
    CalcFuzzy(uint32_t currRate, double currDt, double diff,
        uint32_t & nextRate, Time & b_delay);

    void
    CalcFuzzyv4(uint32_t currRate, double currDt, double throughput,
        uint32_t & nextRate, Time & b_delay);

    void
    CalcAAASH(uint32_t currRate, double currDt, double diff,
        uint32_t & nextRate, Time & b_delay);

    void
    CalcOSMP(uint32_t currRate, double currDt, double diff,
        uint32_t & nextRate, Time & b_delay);

    void
    CalcSVAA(uint32_t currRate, double currDt, double diff,
        uint32_t & nextRate, Time & b_delay);

    bool
    BufferInc();

    Time m_lastpaused;
    std::queue<Ptr<Packet> > m_queue;
    std::map<Time, double> m_bitrates;
    double m_bitrateEstimate;
    bool m_running_fast_start;
    std::map<Time, Time> m_bufferState;
    Time m_bufferDelay;
    DashClient * m_dashClient;
    Protocol m_protocol;
    Time m_window;
    int m_counter;
    int m_m_k_1;
    int m_m_k_2;

  };
} // namespace ns3

#endif /* MPEG_PLAYER_H_ */
