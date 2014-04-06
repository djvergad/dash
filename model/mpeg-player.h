/*
 * mpeg-player.h
 *
 *  Created on: Jan 29, 2014
 *      Author: dimitriv
 */

#ifndef MPEG_PLAYER_H_
#define MPEG_PLAYER_H_

#include <queue>
#include <map>
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

  class DashClient;

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

    void
    CalcNextSegment(uint32_t currRate, double currDt, double diff,
        uint32_t & nextRate, Time & b_delay);

    Time
    CalcSendTime(uint32_t currRate, double currDt, double diff);

    void
    AddBitRate(Time time, double bitrate);

    void
    LogBufferLevel();

    double inline
    GetBitRateEstimate()
    {
      return m_bitrateEstimate;
    }

    void inline
    SchduleBufferWakeup(const Time t, DashClient * client)
    {
      m_bufferDelay = t;
      m_dashClient = client;

    }

    void inline
    SetProtocol(Protocol protocol)
    {
      m_protocol = protocol;

    }

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

    void
    CalcFuzzy(uint32_t currRate, double currDt, double diff,
        uint32_t & nextRate, Time & b_delay);

    void
    CalcAAASH(uint32_t currRate, double currDt, double diff,
        uint32_t & nextRate, Time & b_delay);

    bool
    BufferInc();

    Time m_lastpaused;
    std::queue<Ptr<Packet> > m_queue;
    std::map<Time, double> m_bitrates;
    double m_bitrateEstimate;
    bool m_running_fast_start;
    std::list<Time> m_bufferState;
    Time m_bufferDelay;
    DashClient * m_dashClient;

  };
} // namespace ns3

#endif /* MPEG_PLAYER_H_ */
