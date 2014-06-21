/*
 * sftm-client.cc
 *
 *  Created on: Jun 16, 2014
 *      Author: dimitriv
 */

#include "sftm-client.h"
#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/dash-client.h>

NS_LOG_COMPONENT_DEFINE("SftmClient");

namespace ns3
{
  NS_OBJECT_ENSURE_REGISTERED(SftmClient);

  TypeId
  SftmClient::GetTypeId(void)
  {
    static TypeId tid =
        TypeId("ns3::SftmClient").SetParent<DashClient>().AddConstructor<
            SftmClient>();
    return tid;
  }

  SftmClient::SftmClient() :
      m_rsft_exceeded(false)
  {
    // TODO Auto-generated constructor stub

  }

  SftmClient::~SftmClient()
  {
    // TODO Auto-generated destructor stub
  }

  void
  SftmClient::CalcNextSegment(uint32_t currRate, uint32_t & nextRate,
      Time & delay)
  {
    uint32_t rates[] =
    /*  { 13281, 18593, 26030, 36443, 51020, 71428, 100000, 140000, 195999,
     274399, 384159, 537823 };*/
      { 45000, 89000, 131000, 178000, 221000, 263000, 334000, 396000, 522000,
          595000, 791000, 1033000, 1245000, 1547000, 2134000, 2484000, 3079000,
          3527000, 3840000, 4220000 };

    uint32_t rates_size = sizeof(rates) / sizeof(rates[0]);

    // Media Segment duration
    double msd = MPEG_FRAMES_PER_SEGMENT * MPEG_TIME_BETWEEN_FRAMES / 1000.0;
    double sft = GetSegmentFetchTime(); // Segment fetch time
    double tbmt = m_target_dt.GetSeconds(); // Target buffering media time
    double ts_ns = m_segmentId * msd; // Timestamp of the next segment
    double ts_o = GetPlayer().m_framesPlayed * MPEG_TIME_BETWEEN_FRAMES
        / 1000.0; // Current playback timestamp
    double rsft = ts_ns - ts_o - tbmt; // Remaining segment fetch time
    double rho = 0.75;

    double rsft_s = rho * msd;
    if (rsft < rsft_s && !m_rsft_exceeded)
      {
        rsft = rsft_s;
      }
    else
      {
        m_rsft_exceeded = true;
      }

    double sftm = std::min(msd, rsft) / sft;

    uint32_t rateInd = rates_size; // The index of the current rate
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

    double ec_u =
        rateInd < rates_size - 1 ?
            (1.0 * rates[rateInd + 1] - rates[rateInd]) / rates[rateInd] :
            std::numeric_limits<double>::max();

    double emax_u = 0;
    for (uint32_t i = 0; i < rates_size - 1; i++)
      {
        double e = (1.0 * rates[i + 1] - rates[i]) / rates[i];
        emax_u = std::max(emax_u, e);
      }

    double e_u = std::min(emax_u, 2 * ec_u);

    double ec_d =
        rateInd > 0 ?
            (1.0 * rates[rateInd] - rates[rateInd - 1]) / rates[rateInd] : 1.0;

    double emin_d = std::numeric_limits<double>::max();
    for (uint32_t i = 1; i < rates_size; i++)
      {
        double e = (1.0 * rates[i] - rates[i - 1]) / rates[i];
        emin_d = std::min(emin_d, e);
      }

    double e_d = std::max(2 * emin_d, ec_d);

    if (sftm > 1 + e_u && rateInd < rates_size - 1) // Switch up
      {
        nextRate = rates[rateInd + 1];
      }
    else if (sftm < 1 - e_d && rateInd != 0) // Switch down
      {
        for (uint32_t i = 0; i < rateInd; i++)
          {
            if (rates[i] >= sftm * rates[rateInd])
              {
                break;
              }
            nextRate = rates[i];
          }
      }

    double bmt_min = 0;
    double bmt_c = GetBufferEstimate();
    double b_max = rates[rates_size - 1];
    double b_min = rates[0];
    double t_id = bmt_c - bmt_min - msd * b_max / b_min;

    if (t_id > 0) // Delay
      {
        delay = Seconds(t_id);
      }
    else
      {
        delay = Seconds(0);
      }
  }
} /* namespace ns3 */
