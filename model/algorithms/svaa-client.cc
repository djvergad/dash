/*
 * svaa-client.cc
 *
 *  Created on: Jun 16, 2014
 *      Author: dimitriv
 */

#include "svaa-client.h"
#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/dash-client.h>

NS_LOG_COMPONENT_DEFINE("SvaaClient");

namespace ns3
{
  NS_OBJECT_ENSURE_REGISTERED(SvaaClient);

  TypeId
  SvaaClient::GetTypeId(void)
  {
    static TypeId tid =
        TypeId("ns3::SvaaClient").SetParent<DashClient>().AddConstructor<
            SvaaClient>();
    return tid;
  }

  SvaaClient::SvaaClient() :
      m_m_k_1(0), m_m_k_2(0), m_counter(0)
  {
    // TODO Auto-generated constructor stub

  }

  SvaaClient::~SvaaClient()
  {
    // TODO Auto-generated destructor stub
  }

  void
  SvaaClient::CalcNextSegment(uint32_t currRate, uint32_t & nextRate,
      Time & delay)
  {
    uint32_t rates[] =
      { 45000, 89000, 131000, 178000, 221000, 263000, 334000, 396000, 522000,
          595000, 791000, 1033000, 1245000, 1547000, 2134000, 2484000, 3079000,
          3527000, 3840000, 4220000 };

    uint32_t rates_size = sizeof(rates) / sizeof(rates[0]);
    double diff = GetBufferDifferential();

    double q_tk = GetBufferEstimate();
    double q_ref = m_target_dt.GetSeconds();
    double p = 1.0;
    double f_q = 2 * std::exp(p * (q_tk - q_ref))
        / (1 + std::exp(p * (q_tk - q_ref)));
    double delta = MPEG_TIME_BETWEEN_FRAMES * MPEG_FRAMES_PER_SEGMENT / 1000.0;
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
        delay = Seconds(0);
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
            delay = Seconds(0);
            m_counter = 0;
            return;
          }
      }
    else if (u_k < currRate)
      {
        m_counter = 0;
      }
    nextRate = currRate;
    delay = Seconds(0);
    return;

  }

} /* namespace ns3 */
