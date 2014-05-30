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
          100000000), m_framesPlayed(0), m_target_dt(Seconds(7.0)), m_bitrateEstimate(
          0.0), m_running_fast_start(true), m_bufferDelay("0s"), m_protocol(
          AAASH /*FUZZY*/), m_window(Seconds(10)), m_counter(0), m_m_k_1(0), m_m_k_2(
          0)
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
    case FUZZYv4:
      CalcFuzzyv4(currRate, currDt, m_bitrateEstimate, nextRate, b_delay);

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
  }

  void
  MpegPlayer::CalcSVAA(uint32_t currRate, double currDt, double diff,
      uint32_t & nextRate, Time & b_delay)
  {
    uint32_t rates[] =
      { 45000, 89000, 131000, 178000, 221000, 263000, 334000, 396000, 522000,
          595000, 791000, 1033000, 1245000, 1547000, 2134000, 2484000, 3079000,
          3527000, 3840000, 4220000 };

    uint32_t rates_size = sizeof(rates) / sizeof(rates[0]);

    double q_tk = currDt;
    double q_ref = m_target_dt.GetSeconds();
    double p = 1.0;
    double f_q = 2 * std::exp(p * (q_tk - q_ref))
        / (1 + std::exp(p * (q_tk - q_ref)));
    double delta = TIME_BETWEEN_FRAMES * MPEG_FRAMES_PER_SEGMENT;
    double f_t = delta / (delta - diff);
    double f_u = 1;

    double f = f_q * f_t * f_u;

    double t_k = m_bitrateEstimate;

    double u_k = f * t_k;

    int m_k = 0;
    if (diff >= 0.4 * delta)
      {
        m_k = 1;
      }
    else if (diff >= 0.2 * delta)
      {
        m_k = 5;
      }
    else if (diff >= 0)
      {
        m_k = 15;
      }
    else
      {
        m_k = 20;
      }

    int m = (m_k + m_m_k_1 + m_m_k_2) / 3;
    m_m_k_2 = m_m_k_1;
    m_m_k_1 = m_k;

    if (q_tk < q_ref / 2)
      {
        int i = rates_size - 1;
        while (rates[i] > t_k && i > 0)
          {
            i--;
          }
        nextRate = rates[i];
        b_delay = Seconds(0);
        return;
      }
    else if (u_k > currRate)
      {
        m_counter++;
        if (m_counter > m)
          {
            int i = rates_size - 1;
            while (rates[i] > t_k && i > 0)
              {
                i--;
              }
            nextRate = rates[i];
            b_delay = Seconds(0);
            m_counter = 0;
            return;
          }
      }
    else if (u_k < currRate)
      {
        m_counter = 0;
      }
    nextRate = currRate;
    b_delay = Seconds(0);
    return;
  }

  void
  MpegPlayer::CalcOSMP(uint32_t currRate, double currDt, double diff,
      uint32_t & nextRate, Time & b_delay)
  {

    uint32_t rates[] =
      { 45000, 89000, 131000, 178000, 221000, 263000, 334000, 396000, 522000,
          595000, 791000, 1033000, 1245000, 1547000, 2134000, 2484000, 3079000,
          3527000, 3840000, 4220000 };

    uint32_t rates_size = sizeof(rates) / sizeof(rates[0]);

    std::map<Time, Time>::iterator it;
    it = m_bufferState.end();

    it--;
    Time now = it->first;

    if (it != m_bufferState.begin())
      {
        it--;
      }

    // The delivery time of the last fragment
    Time t_last_frag = now - it->first;

    // The current quality level
    int l_cur = 0;
    while (rates[l_cur] != currRate)
      {
        l_cur++;
      }

    int l_nxt = 0; // The next quality level
    int l_min = 0;  // The lowest quality level
    int l_max = rates_size - 1; // The highest quality level;

    double r_download = 100;
    if (t_last_frag.GetMilliSeconds() > 0)
      {
        r_download = (1.0 * MPEG_FRAMES_PER_SEGMENT * TIME_BETWEEN_FRAMES)
            / t_last_frag.GetMilliSeconds();
      }

    /*    std::cerr << "r_download: " << r_download << "\tratio: "
     << ((1.0 * rates[l_cur - 1]) / rates[l_cur]) << std::endl;*/

    if (r_download < 1)
      {
        if (l_cur > l_min)
          {
            if (r_download < ((1.0 * rates[l_cur - 1]) / rates[l_cur]))
              {
                l_nxt = l_min;
              }
            else
              {
                l_nxt = l_cur - 1;
              }
          }
      }
    else
      {
        if (l_cur < l_max)
          {
            if (l_cur == 0
                || r_download > ((1.0 * rates[l_cur - 1]) / rates[l_cur]))
              {
                do
                  {
                    l_nxt = l_nxt + 1;
                  }
                while (!(l_nxt == l_max
                    || r_download < ((1.0 * rates[l_nxt + 1]) / rates[l_cur])));
              }
          }
      }

    // Pass the results back through the reference variables
    b_delay = Seconds(0);
    nextRate = rates[l_nxt];

  }

  void
  MpegPlayer::CalcAAASH(uint32_t currRate, double currDt, double diff,
      uint32_t & nextRate, Time & b_delay)
  {
    uint32_t rates[] =
    /*  { 13281, 18593, 26030, 36443, 51020, 71428, 100000, 140000, 195999,
     274399, 384159, 537823 };*/
      { 45000, 89000, 131000, 178000, 221000, 263000, 334000, 396000, 522000,
          595000, 791000, 1033000, 1245000, 1547000, 2134000, 2484000, 3079000,
          3527000, 3840000, 4220000 };

    uint32_t rates_size = sizeof(rates) / sizeof(rates[0]);

    double a1 = 0.75;
    double a2 = 0.33;
    double a3 = 0.5;
    double a4 = 0.75;
    double a5 = 0.9;

    Time b_min("10s");
    Time b_low("20s");
    Time b_high("50s");
    Time b_opt = Seconds((b_low + b_high).GetSeconds() * 0.5);

    Time taf(MilliSeconds(MPEG_FRAMES_PER_SEGMENT * TIME_BETWEEN_FRAMES));

    Time b_t = m_bufferState.rbegin()->second;
    // std::cerr << "bt= " << b_t.GetSeconds() << std::endl;

    uint32_t rateInd = rates_size;
    for (uint32_t i = 0; i < rates_size; i++)
      {
        if (rates[i] == currRate)
          {
            rateInd = i;
            break;
          }
      }
    if (rateInd == rates_size)
      {
        NS_FATAL_ERROR("Wrong rate");
      }

    nextRate = currRate;
    b_delay = Seconds(0);

    uint32_t r_up = rates[rateInd];
    uint32_t r_down = rates[rateInd];
    if (rateInd + 1 < rates_size)
      {
        r_up = rates[rateInd + 1];
      }
    if (rateInd >= 1)
      {
        r_down = rates[rateInd - 1];
      }

    /*std::cerr << "bufinc: " << BufferInc() << " FastStart: "
     << m_running_fast_start << std::endl;*/
    if (m_running_fast_start && (rateInd != rates_size - 1) && BufferInc()
        && (currRate <= a1 * m_bitrateEstimate))
      {
        if (b_t < b_min)
          {
            if (r_up <= a2 * m_bitrateEstimate)
              {
                nextRate = r_up;
              }
          }
        else if (b_t < b_low)
          {
            if (r_up <= a3 * m_bitrateEstimate)
              {
                nextRate = r_up;
              }
          }
        else
          {
            if (r_up <= a4 * m_bitrateEstimate)
              {
                nextRate = r_up;
              }
            if (b_t > b_high)
              {
                b_delay = b_high - taf;
              }
          }
      }
    else
      {
        m_running_fast_start = false;
        if (b_t < b_min)
          {
            nextRate = rates[0];
          }
        else if (b_t < b_low)
          {
            if (currRate != rates[0] && currRate >= m_bitrateEstimate)
              {
                nextRate = r_down;
              }
          }
        else if (b_t < b_high)
          {
            if ((currRate == rates[rates_size - 1])
                || (r_up >= a5 * m_bitrateEstimate))
              {
                b_delay = std::max(b_t - taf, b_opt);
              }
          }
        else
          {
            if ((currRate == rates[rates_size - 1])
                || (r_up >= a5 * m_bitrateEstimate))
              {
                b_delay = std::max(b_t - taf, b_opt);
              }
            else
              {
                nextRate = r_up;
              }

          }
      }NS_LOG_INFO(
        "nextRate: " << nextRate << "\tb_delay: " << b_delay.GetSeconds() << "\tb_t: " << b_t.GetSeconds() << "\tb_opt: " << b_opt.GetSeconds());

  }

  bool
  MpegPlayer::BufferInc()
  {
    Time last(Seconds(0));
    //std::cerr << "===" << std::endl;
    for (std::map<Time, Time>::iterator it = m_bufferState.begin();
        it != m_bufferState.end(); it++)
      {
        //  std::cerr << it->first.GetSeconds() << " " << it->second.GetSeconds()
        //      << std::endl;

        if (it->second < last)
          {
            return false;
          }
        last = it->second;
      }
    return true;
  }

  void
  MpegPlayer::CalcFuzzy(uint32_t currRate, double currDt, double diff,
      uint32_t & nextRate, Time & b_delay)
  {
    double slow = 0, ok = 0, fast = 0, falling = 0, steady = 0, rising = 0, r1 =
        0, r2 = 0, r3 = 0, r4 = 0, r5 = 0, r6 = 0, r7 = 0, r8 = 0, r9 = 0, p2 =
        0, p1 = 0, z = 0, n1 = 0, n2 = 0, output = 0;

    double t = m_target_dt.GetSeconds();

    double w = 15; // For FUZZYv3
    double s = 10; // For FUZZYv3

    switch (m_protocol)
      {
    case FUZZY:
    case FUZZYv2:
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
      break;
    case FUZZYv3:
      if (currDt < t - w - s)
        {
          slow = 1;
          ok = 0;
          fast = 0;
        }
      else if (currDt < t - w + s)
        {
          slow = 1 - (currDt - (t - w - s)) / (2 * s);
          ok = (currDt - (t - w - s)) / (2 * s);
          fast = 0;
        }
      else if (currDt < t + w - s)
        {
          slow = 0;
          ok = 1;
          fast = 0;
        }
      else if (currDt < t + w + s)
        {
          slow = 0;
          ok = 1 - (currDt - (t + w - s)) / (2 * s);
          fast = (currDt - (t + w - s)) / (2 * s);
        }
      else
        {
          slow = 0;
          ok = 0;
          fast = 1;
        }

      break;
    case AAASH:
    default:
      NS_FATAL_ERROR("Wrong protocol.");
      break;
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

    /*output = (n2 * 0.25 + n1 * 0.5 + z * 1 + p1 * 1.4 + p2 * 2)*/
    output = (n2 * 0.25 + n1 * 0.5 + z * 1 + p1 * 2 + p2 * 4)
        / (n2 + n1 + z + p1 + p2);

    NS_LOG_INFO(
        "currDt: " << currDt << " diff: " << diff << " slow: " << slow << " ok: " << ok << " fast: " << fast << " falling: " << falling << " steady: " << steady << " rising: " << rising << " r1: " << r1 << " r2: " << r2 << " r3: " << r3 << " r4: " << r4 << " r5: " << r5 << " r6: " << r6 << " r7: " << r7 << " r8: " << r8 << " r9: " << r9 << " p2: " << p2 << " p1: " << p1 << " z: " << z << " n1: " << n1 << " n2: " << n2 << " output: " << output);

    uint32_t result = 0;

    switch (m_protocol)
      {
    case FUZZY:
      result = output * currRate;
      break;
    case FUZZYv2:
    case FUZZYv3:
      result = output * m_bitrateEstimate;
      break;
    default:
      NS_FATAL_ERROR("Wrong Protocol");
      }

    uint32_t rates[] =
    /*  { 13281, 18593, 26030, 36443, 51020, 71428, 100000, 140000, 195999,
     274399, 384159, 537823 };*/
      { 45000, 89000, 131000, 178000, 221000, 263000, 334000, 396000, 522000,
          595000, 791000, 1033000, 1245000, 1547000, 2134000, 2484000, 3079000,
          3527000, 3840000, 4220000 };

    uint32_t rates_size = sizeof(rates) / sizeof(rates[0]);

    uint32_t i;

    nextRate = rates[0];

    for (i = 0; i < rates_size; i++)
      {
        if (result > rates[i])
          {
            nextRate = rates[i];
          }
      }

    b_delay = Seconds(0);
    if (nextRate > currRate)
      {
        /*
         double incrProb = std::pow(0.8,
         (std::log10((double) currRate) - 5) / std::log10(1.4));

         UniformVariable uv;
         double rand = uv.GetValue();
         NS_LOG_INFO(currRate << " " << incrProb << " " << rand);
         if (rand > incrProb)
         {
         nextRate = currRate;
         }
         */
        double t_60 = currDt + (m_bitrateEstimate / nextRate - 1) * 60;
        /*std::cerr << "bef: " << t_60 << std::endl;*/
        if (t_60 < t)
          {
            nextRate = currRate;
            t_60 = currDt + (m_bitrateEstimate / nextRate - 1) * 60;
            /*std::cerr << "aft: " << t_60 << std::endl;*/
            if (t_60 > t)
              {
                b_delay = Seconds(t_60 - t);
              }
          }
        /*std::cerr << b_delay.GetSeconds() << std::endl;*/
      }
    else if (nextRate < currRate)
      {
        double t_60 = currDt + (m_bitrateEstimate / nextRate - 1) * 60;
        //std::cerr << "bef: " << t_60 << std::endl;
        if (t_60 > t)
          {
            t_60 = currDt + (m_bitrateEstimate / currRate - 1) * 60;
            if (t_60 > t)
              {
                nextRate = currRate;
              }
            //   std::cerr << "aft: " << t_60 << std::endl;
          }
      }

    NS_LOG_INFO(currRate << " " << output << " " << result);

    /*result = result > 100000 ? result : 100000;
     result = result < 400000 ? result : 400000;*/

  }

  void
  MpegPlayer::CalcFuzzyv4(uint32_t currRate, double currDt, double throughput,
      uint32_t & nextRate, Time & b_delay)
  {
    double slow = 0, ok = 0, fast = 0, under_rate = 0, at_rate = 0, over_rate =
        0, r1 = 0, r2 = 0, r3 = 0, r4 = 0, r5 = 0, r6 = 0, r7 = 0, r8 = 0, r9 =
        0, p2 = 0, p1 = 0, z = 0, n1 = 0, n2 = 0, output = 0;

    double t = m_target_dt.GetSeconds();

    double w = 15;
    double s = 10;
    if (currDt < t - w - s)
      {
        slow = 1;
        ok = 0;
        fast = 0;
      }
    else if (currDt < t - w + s)
      {
        slow = 1 - (currDt - (t - w - s)) / (2 * s);
        ok = (currDt - (t - w - s)) / (2 * s);
        fast = 0;
      }
    else if (currDt < t + w - s)
      {
        slow = 0;
        ok = 1;
        fast = 0;
      }
    else if (currDt < t + w + s)
      {
        slow = 0;
        ok = 1 - (currDt - (t + w - s)) / (2 * s);
        fast = (currDt - (t + w - s)) / (2 * s);
      }
    else
      {
        slow = 0;
        ok = 0;
        fast = 1;
      }

    double th_low = currRate / 2;
    double th_high = 2 * currRate;
    double w_low = currRate / 4;
    double w_high = currRate / 4;

    if (throughput < th_low - w_low)
      {
        under_rate = 1;
        at_rate = 0;
        over_rate = 0;
      }
    else if (throughput < th_low + w_low)
      {
        under_rate = 1 - (throughput - (th_low - w_low)) / (2 * w_low);
        at_rate = (throughput - (th_low - w_low)) / (2 * w_low);
        over_rate = 0;
      }
    else if (throughput < th_high - w_high)
      {
        under_rate = 0;
        at_rate = 1;
        over_rate = 0;
      }
    else if (throughput < th_high + w_high)
      {
        under_rate = 0;
        at_rate = 1 - (throughput - (th_high - w_high)) / (2 * w_high);
        over_rate = (throughput - (th_high - w_high)) / (2 * w_high);
      }
    else
      {
        under_rate = 0;
        at_rate = 0;
        over_rate = 1;
      }

    r1 = std::min(slow, under_rate);
    r2 = std::min(ok, under_rate);
    r3 = std::min(fast, under_rate);
    r4 = std::min(slow, at_rate);
    r5 = std::min(ok, at_rate);
    r6 = std::min(fast, at_rate);
    r7 = std::min(slow, over_rate);
    r8 = std::min(ok, over_rate);
    r9 = std::min(fast, over_rate);

    p2 = std::sqrt(std::pow(r9, 2));
    p1 = std::sqrt(std::pow(r6, 2) + std::pow(r8, 2));
    z = std::sqrt(std::pow(r3, 2) + std::pow(r5, 2) + std::pow(r7, 2));
    n1 = std::sqrt(std::pow(r2, 2) + std::pow(r4, 2));
    n2 = std::sqrt(std::pow(r1, 2));

    /*output = (n2 * 0.25 + n1 * 0.5 + z * 1 + p1 * 1.4 + p2 * 2)*/
    output = (n2 * 0.25 + n1 * 0.5 + z * 1 + p1 * 2 + p2 * 4)
        / (n2 + n1 + z + p1 + p2);
    /*

     std::cerr << "currDt: " << currDt << " throughput: " << throughput
     << " slow: " << slow << " ok: " << ok << " fast: " << fast
     << " under_rate: " << under_rate << " at_rate: " << at_rate
     << " over_rate: " << over_rate << " r1: " << r1 << " r2: " << r2
     << " r3: " << r3 << " r4: " << r4 << " r5: " << r5 << " r6: " << r6
     << " r7: " << r7 << " r8: " << r8 << " r9: " << r9 << " p2: " << p2
     << " p1: " << p1 << " z: " << z << " n1: " << n1 << " n2: " << n2
     << " output: " << output << std::endl;
     */

    uint32_t result = output * currRate;

    uint32_t rates[] =
    /*  { 13281, 18593, 26030, 36443, 51020, 71428, 100000, 140000, 195999,
     274399, 384159, 537823 };*/
      { 45000, 89000, 131000, 178000, 221000, 263000, 334000, 396000, 522000,
          595000, 791000, 1033000, 1245000, 1547000, 2134000, 2484000, 3079000,
          3527000, 3840000, 4220000 };

    uint32_t rates_size = sizeof(rates) / sizeof(rates[0]);

    uint32_t i, k = 0;

    nextRate = rates[0];

    for (i = 0; i < rates_size; i++)
      {
        if (result > rates[i])
          {
            nextRate = rates[i];
            k = i;
          }
      }

    if (nextRate > currRate)
      {
        for (i = k; i >= 0; i--)
          {
            double t_60 = currDt + (throughput / nextRate - 1) * 60;
            /* std::cerr << k << " " << rates[k] << " " << nextRate << " " << t_60
             << std::endl;*/
            if (t_60 > t)
              {
                break;
              }
            else if (i > 0)
              {
                nextRate = rates[i - 1];
              }
          }

      }

    b_delay = Seconds(0);

    NS_LOG_INFO(currRate << " " << output << " " << result);

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

  void
  MpegPlayer::LogBufferLevel(Time t)
  {
    m_bufferState[Simulator::Now()] = t;
    for (std::map<Time, Time>::iterator it = m_bufferState.begin();
        it != m_bufferState.end(); ++it)
      {
        if (it->first < (Simulator::Now() - m_window))
          {
            m_bufferState.erase(it->first);
          }
      }
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
        std::cerr << Simulator::Now().GetSeconds() << " No frames to play"
            << std::endl;
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
