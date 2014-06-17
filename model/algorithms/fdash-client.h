/*
 * fdash-client.h
 *
 *  Created on: Jun 16, 2014
 *      Author: dimitriv
 */

#ifndef FDASH_CLIENT_H_
#define FDASH_CLIENT_H_

#include <ns3/dash-client.h>
namespace ns3
{

  class FdashClient : public DashClient
  {
  public:
    static TypeId
    GetTypeId(void);

    FdashClient();

    virtual
    ~FdashClient();

    virtual void
    CalcNextSegment(uint32_t currRate, uint32_t & nextRate, Time & delay);

  private:
    bool
    BufferInc();

  };

} /* namespace ns3 */

#endif /* FDASH_CLIENT_H_ */
