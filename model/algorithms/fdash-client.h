/*
 * fdash-client.h
 *
 *  Created on: Jun 16, 2014
 *      Author: dimitriv
 */

#ifndef FDASH_CLIENT_H_
#define FDASH_CLIENT_H_

#include <ns3/dash-client.h>
namespace ns3 {

class FdashClient : public DashClient
{
  friend class MpegPlayer;
  friend class FrameBuffer;

public:
  static TypeId GetTypeId (void);

  FdashClient ();

  virtual ~FdashClient ();

  virtual void CalcNextSegment (uint32_t currRate, uint32_t &nextRate, Time &delay);

  virtual void ForecastDisruption (Time estimatedTime, Time estimatedDuration);

  virtual void CalcedDisruption ();

private:
  bool BufferInc ();

  double m_interruptionLimit = rates.back ();
};

} /* namespace ns3 */

#endif /* FDASH_CLIENT_H_ */
