/*
 * fdash-client.cc
 *
 *  Created on: Jun 16, 2014
 *      Author: dimitriv
 */

#include "fdash-client.h"
#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/dash-client.h>

NS_LOG_COMPONENT_DEFINE("FdashClient");

namespace ns3
{
  NS_OBJECT_ENSURE_REGISTERED(FdashClient);

  TypeId
  FdashClient::GetTypeId(void)
  {
    static TypeId tid =
        TypeId("ns3::FdashClient").SetParent<DashClient>().AddConstructor<
            FdashClient>();
    return tid;
  }

  FdashClient::FdashClient()
  {
    // TODO Auto-generated constructor stub

  }

  FdashClient::~FdashClient()
  {
    // TODO Auto-generated destructor stub
  }

  void
  FdashClient::CalcNextSegment(uint32_t currRate, uint32_t & nextRate,
      Time & delay)
  {
    double slow = 0, ok = 0, fast = 0, falling = 0, steady = 0, rising = 0, r1 =
        0, r2 = 0, r3 = 0, r4 = 0, r5 = 0, r6 = 0, r7 = 0, r8 = 0, r9 = 0, p2 =
        0, p1 = 0, z = 0, n1 = 0, n2 = 0, output = 0;

    double t = m_target_dt.GetSeconds();

    double currDt = GetBufferEstimate();
    double diff = GetBufferDifferential();

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

    /*output = (n2 * 0.25 + n1 * 0.5 + z * 1 + p1 * 1.4 + p2 * 2)*/
    output = (n2 * 0.25 + n1 * 0.5 + z * 1 + p1 * 2 + p2 * 4)
        / (n2 + n1 + z + p1 + p2);

    NS_LOG_INFO(
        "currDt: " << currDt << " diff: " << diff << " slow: " << slow << " ok: " << ok << " fast: " << fast << " falling: " << falling << " steady: " << steady << " rising: " << rising << " r1: " << r1 << " r2: " << r2 << " r3: " << r3 << " r4: " << r4 << " r5: " << r5 << " r6: " << r6 << " r7: " << r7 << " r8: " << r8 << " r9: " << r9 << " p2: " << p2 << " p1: " << p1 << " z: " << z << " n1: " << n1 << " n2: " << n2 << " output: " << output);

    uint32_t result = 0;

    result = output * m_bitrateEstimate;

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

    delay = Seconds(0);
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
                // delay = Seconds(t_60 - t);
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

} /* namespace ns3 */
