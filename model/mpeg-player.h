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

#define MPEG_PLAYER_PAUSED 1
#define MPEG_PLAYER_PLAYING 2
#define MPEG_PLAYER_NOT_STARTED 3

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
    /*uint32_t CalcSendRate(uint32_t recvRate, Time dt1);*/
    uint32_t
    CalcSendRate(uint32_t currRate, double currDt, double diff);

    int m_state;
    Time m_interruption_time;
    int m_interrruptions;

    Time m_start_time;
    uint32_t m_totalRate;
    uint32_t m_minRate;
    uint32_t m_framesPlayed;
    Time m_target_dt;

  private:
    void
    PlayFrame();

    Time m_lastpaused;
    std::queue<Ptr<Packet> > m_queue;

  };
} // namespace ns3

#endif /* MPEG_PLAYER_H_ */
