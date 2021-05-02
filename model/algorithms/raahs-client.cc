/*
 * raahs-client.cc
 *
 *  Created on: Jun 16, 2014
 *      Author: dimitriv
 */

#include "raahs-client.h"
#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/dash-client.h>

NS_LOG_COMPONENT_DEFINE ("RaahsClient");

namespace ns3 {
NS_OBJECT_ENSURE_REGISTERED (RaahsClient);

TypeId
RaahsClient::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::RaahsClient").SetParent<DashClient> ().AddConstructor<RaahsClient> ();
  return tid;
}

RaahsClient::RaahsClient ()
{
  // TODO Auto-generated constructor stub
}

RaahsClient::~RaahsClient ()
{
  // TODO Auto-generated destructor stub
}

void
RaahsClient::CalcNextSegment (uint32_t currRate, uint32_t &nextRate, Time &delay)
{

  // Media Segment duration
  double msd = MPEG_FRAMES_PER_SEGMENT * MPEG_TIME_BETWEEN_FRAMES / 1000.0;
  double sft = GetSegmentFetchTime (); // Segment fetch time

  double mi = msd / sft;

  double t_min = 9; // Seconds

  double epsilon = 0.0;
  uint32_t i;
  for (i = 0; i < rates.size () - 1; i++)
    {
      epsilon = std::max (epsilon, (1.0 * rates[i + 1] - rates[i]) / rates[i]);
    }

  double gamma_d = 0.67; // Switch down factor

  double rateInd = rates.size ();
  for (uint32_t i = 0; i < rates.size (); i++)
    {
      if (rates[i] == currRate)
        {
          rateInd = i;
          break;
        }
    }
  if (rateInd == rates.size ())
    {
      NS_FATAL_ERROR ("Wrong rate");
    }

  if (mi > 1 + epsilon) // Switch Up
    {
      if (rateInd < rates.size () - 1)
        {
          nextRate = rates[(int) rateInd + 1];
        }
    }
  else if (mi < gamma_d) // Switch down
    {
      i = rateInd - 1;
      for (i = 0; i < rateInd - 1; i--)
        {
          if (rates[i] < mi * currRate)
            {
              nextRate = rates[i];
            }
          else
            {
              break;
            }
        }
    }

  // Calculate delay;
  double ts = GetBufferEstimate () - t_min - currRate * msd / rates[0];

  if (ts > 0)
    {
      delay = Seconds (ts);
    }
  else
    {
      delay = Seconds (0);
    }
}
} /* namespace ns3 */
