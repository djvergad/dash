/*
 * svaa-client.h
 *
 *  Created on: Jun 16, 2014
 *      Author: dimitriv
 */

#ifndef SVAA_CLIENT_H_
#define SVAA_CLIENT_H_

#include <ns3/dash-client.h>
namespace ns3
{

  class SvaaClient : public DashClient
  {
  public:
    static TypeId
    GetTypeId(void);

    SvaaClient();

    virtual
    ~SvaaClient();

    virtual void
    CalcNextSegment(uint32_t currRate, uint32_t & nextRate, Time & delay);

  private:
    int m_m_k_1;
    int m_m_k_2;
    int m_counter;
  };

} /* namespace ns3 */

#endif /* SVAA_CLIENT_H_ */
