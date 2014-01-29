/*
 * mpeg-player.h
 *
 *  Created on: Jan 29, 2014
 *      Author: dimitriv
 */

#ifndef MPEG_PLAYER_H_
#define MPEG_PLAYER_H_

#include <queue>
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
    FUZZY, AAASH
  };

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

    uint32_t
    CalcSendRate(uint32_t currRate, double currDt, double diff);

    Time
    CalcSendTime(uint32_t currRate, double currDt, double diff);

    int m_state;
    Time m_interruption_time;
    int m_interrruptions;

    Time m_start_time;
    uint32_t m_totalRate;
    uint32_t m_minRate;
    uint32_t m_framesPlayed;
    Time m_target_dt;
    Protocol m_protocol;

  private:
    void
    PlayFrame();

    uint32_t
    CalcFuzzy(uint32_t currRate, double currDt, double diff);

    uint32_t
    CalcAAASH(uint32_t currRate, double currDt, double diff);

    Time m_lastpaused;
    std::queue<Ptr<Packet> > m_queue;

  };
} // namespace ns3

#endif /* MPEG_PLAYER_H_ */
