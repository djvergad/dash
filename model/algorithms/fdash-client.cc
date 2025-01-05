/*
 * fdash-client.cc
 *
 *  Created on: Jun 16, 2014
 *      Author: dimitriv
 */

#include "fdash-client.h"

#include "../mpeg-header.h"

#include "ns3/data-rate.h"
#include <ns3/dash-client.h>
#include <ns3/log.h>
#include <ns3/simulator.h>

NS_LOG_COMPONENT_DEFINE("FdashClient");

namespace ns3
{
NS_OBJECT_ENSURE_REGISTERED(FdashClient);

TypeId
FdashClient::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::FdashClient").SetParent<DashClient>().AddConstructor<FdashClient>();
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
FdashClient::ForecastDisruption(Time estimatedTime, Time estimatedDuration)
{
    Time bufferTime = MilliSeconds(MPEG_TIME_BETWEEN_FRAMES) * m_player.m_frameBuffer.size();

    double can_download_bits = m_bitrateEstimate * estimatedTime.GetSeconds();

    double newRate =
        can_download_bits / (m_target_dt + estimatedDuration - bufferTime).GetSeconds();

    m_interruptionLimit = newRate;

    // DataRate m_emergency_bitrate =;
}

void
FdashClient::CalcedDisruption()
{
    m_interruptionLimit = rates.back();
}

void
FdashClient::CalcNextSegment(uint32_t currRate, uint32_t& nextRate, Time& delay)
{
    double slow = 0, ok = 0, fast = 0, falling = 0, steady = 0, rising = 0, r1 = 0, r2 = 0, r3 = 0,
           r4 = 0, r5 = 0, r6 = 0, r7 = 0, r8 = 0, r9 = 0, p2 = 0, p1 = 0, z = 0, n1 = 0, n2 = 0,
           output = 0;

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
    output = (n2 * 0.25 + n1 * 0.5 + z * 1 + p1 * 2 + p2 * 4) / (n2 + n1 + z + p1 + p2);

    NS_LOG_INFO("currDt: " << currDt << " diff: " << diff << " slow: " << slow << " ok: " << ok
                           << " fast: " << fast << " falling: " << falling << " steady: " << steady
                           << " rising: " << rising << " r1: " << r1 << " r2: " << r2
                           << " r3: " << r3 << " r4: " << r4 << " r5: " << r5 << " r6: " << r6
                           << " r7: " << r7 << " r8: " << r8 << " r9: " << r9 << " p2: " << p2
                           << " p1: " << p1 << " z: " << z << " n1: " << n1 << " n2: " << n2
                           << " output: " << output);

    uint32_t result = 0;

    result = output * m_bitrateEstimate;

    if (result > m_interruptionLimit)
    {
        result = m_interruptionLimit;
    }

    nextRate = rates.front();

    for (uint32_t rate : rates)
    {
        NS_LOG_DEBUG("result: " << result << " rate: " << rate);
        if (result > rate)
        {
            NS_LOG_DEBUG("nextRate: " << nextRate);

            nextRate = rate;
        }
    }

    delay = Seconds(0);
    if (nextRate > currRate)
    {
        double t_lim = currDt + (m_bitrateEstimate / nextRate - 1) * 2*t;
        /*std::cerr << "bef: " << t_lim << std::endl;*/
        if (t_lim < t)
        {
            nextRate = currRate;
            t_lim = currDt + (m_bitrateEstimate / nextRate - 1) * 2*t;
            /*std::cerr << "aft: " << t_lim << std::endl;*/
            if (t_lim > t)
            {
                // delay = Seconds(t_lim - t);
            }
        }
        /*std::cerr << b_delay.GetSeconds() << std::endl;*/
    }
    else if (nextRate < currRate && m_interruptionLimit == rates.back())
    {
        double t_lim = currDt + (m_bitrateEstimate / nextRate - 1) * 2 * t;
        // std::cerr << "bef: " << t_lim << std::endl;
        if (t_lim > t)
        {
            t_lim = currDt + (m_bitrateEstimate / currRate - 1) * 2 * t;
            if (t_lim > t)
            {
                nextRate = currRate;
            }
            //   std::cerr << "aft: " << t_lim << std::endl;
        }
    }

    if (nextRate == rates.back())
    {
        double time_to_download = nextRate * MPEG_TIME_BETWEEN_FRAMES * MPEG_FRAMES_PER_SEGMENT /
                                  1000.0 / m_bitrateEstimate;
        double sleep_time = currDt - t - time_to_download;
        NS_LOG_INFO("time_to_download =  " << time_to_download << " sleep_time =  " << sleep_time);
        if (sleep_time > 0)
        {
            delay = Seconds(sleep_time);
        }
    }

    NS_LOG_INFO(currRate << " " << output << " " << result);

    /*result = result > 100000 ? result : 100000;
       result = result < 400000 ? result : 400000;*/
}

} /* namespace ns3 */
