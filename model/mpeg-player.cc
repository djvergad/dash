/*
 * mpeg-player.cc
 *
 *  Created on: Jan 29, 2014
 *      Author: dimitriv
 */

#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/random-variable.h"
#include "http-header.h"
#include "mpeg-header.h"
#include "mpeg-player.h"
#include <cmath>

NS_LOG_COMPONENT_DEFINE("MpegPlayer");
namespace ns3
{

  MpegPlayer::MpegPlayer() :
      m_state(MPEG_PLAYER_NOT_STARTED), m_interrruptions(0), m_totalRate(0), m_minRate(
          100000000), m_target_dt(Seconds(7.0)), m_protocol(FUZZY)
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
        " Start: " << m_start_time.GetSeconds() << " Inter: "
        << m_interruption_time.GetSeconds() << " playtime: "
        << playTime.GetSeconds() << " now: "
        << Simulator::Now().GetSeconds() << " actual: "
        << (m_start_time + m_interruption_time + playTime).GetSeconds());

    return m_start_time + m_interruption_time
        + (m_state == MPEG_PLAYER_PAUSED ?
            (Simulator::Now() - m_lastpaused) : Seconds(0)) + playTime
        - Simulator::Now();
  }

  /*uint32_t
   MpegPlayer::CalcSendRate(uint32_t recvRate, Time dt1)
   {
   Time target = m_target_dt;

   if (dt1 > 2 * m_target_dt) {
   target =  Seconds(dt1.GetSeconds() / 2);
   }

   double factor = (1-(target - dt1).GetSeconds()/(MPEG_FRAMES_PER_SEGMENT * 0.020));

   if (factor > 1) {
   factor /= 2;
   }

   if (factor < 0) {
   factor = 0.0001;
   }

   return factor * recvRate / 2;

   std::cout << recvRate << " " << dt1.GetSeconds() << " " << factor << " " << factor * recvRate << std::endl;
   return factor * recvRate > 500000? 500000 : factor *recvRate  ;

   }*/

  uint32_t
  MpegPlayer::CalcSendRate(uint32_t currRate, double currDt, double diff)
  {
    switch (m_protocol)
      {
    case FUZZY:
      return CalcFuzzy(currRate, currDt, diff);
      break;
    case AAASH:
      return CalcAAASH(currRate, currDt, diff);
      break;
    default:
      NS_LOG_ERROR("Wrong protocol");
      Simulator::Stop();
      return -1;
      }
  }

  uint32_t
  MpegPlayer::CalcAAASH(uint32_t currRate, double currDt, double diff)
  {
    return currRate;
  }

  uint32_t
  MpegPlayer::CalcFuzzy(uint32_t currRate, double currDt, double diff)
  {
    double slow = 0, ok = 0, fast = 0, falling = 0, steady = 0, rising = 0, r1 =
        0, r2 = 0, r3 = 0, r4 = 0, r5 = 0, r6 = 0, r7 = 0, r8 = 0, r9 = 0, p2 =
        0, p1 = 0, z = 0, n1 = 0, n2 = 0, output = 0;

    double t = m_target_dt.GetSeconds();
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

    output = (n2 * 0.25 + n1 * 0.5 + z * 1 + p1 * 1.4 + p2 * 2)
        / (n2 + n1 + z + p1 + p2);

    NS_LOG_INFO(
        "currDt: " << currDt << " diff: " << diff << " slow: " << slow
        << " ok: " << ok << " fast: " << fast << " falling: " << falling
        << " steady: " << steady << " rising: " << rising << " r1: " << r1
        << " r2: " << r2 << " r3: " << r3 << " r4: " << r4 << " r5: " << r5
        << " r6: " << r6 << " r7: " << r7 << " r8: " << r8 << " r9: " << r9
        << " p2: " << p2 << " p1: " << p1 << " z: " << z << " n1: " << n1
        << " n2: " << n2 << " output: " << output);

    uint32_t result = output * currRate;

    uint32_t nextRate = result > 537823 ? 537823 : result > 384159 ? 384159 :
                        result > 274399 ? 274399 : result > 195999 ? 195999 :
                        result > 140000 ? 140000 : result > 100000 ? 100000 :
                        result > 71428 ? 71428 : result > 51020 ? 51020 :
                        result > 36443 ? 36443 : result > 26030 ? 26030 :
                        result > 18593 ? 18593 : 13281;

    if (nextRate > currRate)
      {
        double incrProb = std::pow(0.8,
            (std::log10((double) currRate) - 5) / std::log10(1.4));

        UniformVariable uv;
        double rand = uv.GetValue();
        NS_LOG_INFO(currRate << " " << incrProb << " " << rand);
        if (rand > incrProb)
          {
            nextRate = currRate;
          }

      }

    NS_LOG_INFO(currRate << " " << output << " " << result);

    /*result = result > 100000 ? result : 100000;
     result = result < 400000 ? result : 400000;*/

    return nextRate;
  }

  Time
  MpegPlayer::CalcSendTime(uint32_t currRate, double currDt, double diff)
  {
    return Seconds(0);
  }

  void
  MpegPlayer::ReceiveFrame(Ptr<Packet> message)
  {
    NS_LOG_FUNCTION(this << message);

    Ptr<Packet> msg = message->Copy();

    m_queue.push(msg);
    if (m_state == MPEG_PLAYER_PAUSED)
      {
        m_state = MPEG_PLAYER_PLAYING;
        m_interruption_time += (Simulator::Now() - m_lastpaused);
        PlayFrame();
      }

  }

  void
  MpegPlayer::Start(void)
  {
    NS_LOG_FUNCTION(this);
    m_state = MPEG_PLAYER_PLAYING;
    m_interruption_time = Seconds(0);
    m_start_time = Simulator::Now() + m_target_dt;
    Simulator::Schedule(m_target_dt, &MpegPlayer::PlayFrame, this);
  }

  void
  MpegPlayer::PlayFrame(void)
  {
    NS_LOG_FUNCTION(this);
    if (m_queue.empty())
      {
        NS_LOG_INFO("No frames to play");
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

    NS_LOG_INFO(
        Simulator::Now().GetSeconds() << " PLAYING FRAME: " << " VidId: "
        << http_header.GetVideoId() << " SegId: "
        << http_header.GetSegmentId() << " Res: "
        << http_header.GetResolution() << " FrameId: "
        << mpeg_header.GetFrameId() << " PlayTime: "
        << mpeg_header.GetPlaybackTime().GetSeconds() << " Type: "
        << (char) mpeg_header.GetType() << " interTime: "
        << m_interruption_time.GetSeconds() << " queueLength: "
        << m_queue.size());

    /*   std::cout << " frId: " << mpeg_header.GetFrameId()
     << " playtime: " << mpeg_header.GetPlaybackTime()
     << " target: " << (m_start_time + m_interruption_time + mpeg_header.GetPlaybackTime()).GetSeconds()
     << " now: " << Simulator::Now().GetSeconds()
     << std::endl;
     */
    Simulator::Schedule(MilliSeconds(20), &MpegPlayer::PlayFrame, this);

  }

} // namespace ns3
