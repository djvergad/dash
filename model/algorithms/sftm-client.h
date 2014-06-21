/*
 * sftm-client.h
 *
 *  Created on: Jun 16, 2014
 *      Author: dimitriv
 */

#ifndef SFTM_CLIENT_H_
#define SFTM_CLIENT_H_

#include <ns3/dash-client.h>
namespace ns3
{

  class SftmClient : public DashClient
  {
  public:
    static TypeId
    GetTypeId(void);

    SftmClient();

    virtual
    ~SftmClient();

    virtual void
    CalcNextSegment(uint32_t currRate, uint32_t & nextRate, Time & delay);

  private:
    bool m_rsft_exceeded;

  };

} /* namespace ns3 */

#endif /* SFTM_CLIENT_H_ */
