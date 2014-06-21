/*
 * osmp-client.cc
 *
 *  Created on: Jun 16, 2014
 *      Author: dimitriv
 */

#include "osmp-client.h"
#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/dash-client.h>

NS_LOG_COMPONENT_DEFINE("OsmpClient");

namespace ns3
{
  NS_OBJECT_ENSURE_REGISTERED(OsmpClient);

  TypeId
  OsmpClient::GetTypeId(void)
  {
    static TypeId tid =
        TypeId("ns3::OsmpClient").SetParent<DashClient>().AddConstructor<
            OsmpClient>();
    return tid;
  }

  OsmpClient::OsmpClient()
  {
    // TODO Auto-generated constructor stub

  }

  OsmpClient::~OsmpClient()
  {
    // TODO Auto-generated destructor stub
  }

  void
  OsmpClient::CalcNextSegment(uint32_t currRate, uint32_t & nextRate,
      Time & delay)
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
        r_download = (1.0 * MPEG_FRAMES_PER_SEGMENT * MPEG_TIME_BETWEEN_FRAMES)
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
    delay = Seconds(0);
    nextRate = rates[l_nxt];

  }

} /* namespace ns3 */
