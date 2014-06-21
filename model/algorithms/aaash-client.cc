/*
 * aaash-client.cc
 *
 *  Created on: Jun 16, 2014
 *      Author: dimitriv
 */

#include "aaash-client.h"
#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/dash-client.h>

NS_LOG_COMPONENT_DEFINE("AaashClient");

namespace ns3
{
  NS_OBJECT_ENSURE_REGISTERED(AaashClient);

  TypeId
  AaashClient::GetTypeId(void)
  {
    static TypeId tid =
        TypeId("ns3::AaashClient").SetParent<DashClient>().AddConstructor<
            AaashClient>();
    return tid;
  }

  AaashClient::AaashClient() :
      m_running_fast_start(true)
  {
    // TODO Auto-generated constructor stub

  }

  AaashClient::~AaashClient()
  {
    // TODO Auto-generated destructor stub
  }

  void
  AaashClient::CalcNextSegment(uint32_t currRate, uint32_t & nextRate,
      Time & delay)
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

    Time taf(MilliSeconds(MPEG_FRAMES_PER_SEGMENT * MPEG_TIME_BETWEEN_FRAMES));

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
    delay = Seconds(0);

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
                delay = b_high - taf;
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
                delay = std::max(b_t - taf, b_opt);
              }
          }
        else
          {
            if ((currRate == rates[rates_size - 1])
                || (r_up >= a5 * m_bitrateEstimate))
              {
                delay = std::max(b_t - taf, b_opt);
              }
            else
              {
                nextRate = r_up;
              }

          }
      }

    NS_LOG_INFO(
        "nextRate: " << nextRate << "\tb_delay: " << delay.GetSeconds() << "\tb_t: " << b_t.GetSeconds() << "\tb_opt: " << b_opt.GetSeconds());

  }
  bool
  AaashClient::BufferInc()
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

} /* namespace ns3 */
