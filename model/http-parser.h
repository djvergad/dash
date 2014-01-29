/*
 * http-parser.h
 *
 *  Created on: Jan 29, 2014
 *      Author: dimitriv
 */

#ifndef HTTP_PARSER_H_
#define HTTP_PARSER_H_

#include <ns3/ptr.h>
#include "mpeg-header.h"

namespace ns3
{

  class Socket;
  class DashClient;

  class HttpParser
  {
  public:
    HttpParser();
    virtual
    ~HttpParser();
    void
    ReadSocket(Ptr<Socket> socket);
    void
    SetApp(DashClient *app);

  private:
    uint8_t m_buffer[MPEG_MAX_MESSAGE];
    uint32_t m_bytes;
    DashClient *m_app;

  };

} // namespace ns3

#endif /* HTTP_PARSER_H_ */
