/*
 * raahs-client.h
 *
 *  Created on: Jun 16, 2014
 *      Author: dimitriv
 */

#ifndef RAAHS_CLIENT_H_
#define RAAHS_CLIENT_H_

#include <ns3/dash-client.h>
namespace ns3
{

  class RaahsClient : public DashClient
  {
  public:
    static TypeId
    GetTypeId(void);

    RaahsClient();

    virtual
    ~RaahsClient();

    virtual void
    CalcNextSegment(uint32_t currRate, uint32_t & nextRate, Time & delay);

  private:

  };

} /* namespace ns3 */

#endif /* RAAHS_CLIENT_H_ */
