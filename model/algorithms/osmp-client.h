/*
 * osmp-client.h
 *
 *  Created on: Jun 16, 2014
 *      Author: dimitriv
 */

#ifndef OSMP_CLIENT_H_
#define OSMP_CLIENT_H_

#include <ns3/dash-client.h>
namespace ns3
{

  class OsmpClient : public DashClient
  {
  public:
    static TypeId
    GetTypeId(void);

    OsmpClient();

    virtual
    ~OsmpClient();

    virtual void
        CalcNextSegment(uint32_t currRate, uint32_t & nextRate, Time & delay);
  };

} /* namespace ns3 */

#endif /* OSMP_CLIENT_H_ */
