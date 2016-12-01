/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 TEI of Western Macedonia, Greece
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Dimitrios J. Vergados <djvergad@gmail.com>
 */

#include "http-parser.h"
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/socket.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "http-header.h"
#include "mpeg-header.h"
#include "dash-client.h"

NS_LOG_COMPONENT_DEFINE("HttpParser");

namespace ns3
{

  HttpParser::HttpParser() :
      m_bytes(0), m_app(NULL), m_lastmeasurement("0s")
  {
    NS_LOG_FUNCTION(this);
  }

  HttpParser::~HttpParser()
  {
    NS_LOG_FUNCTION(this);
  }

  void
  HttpParser::SetApp(DashClient *app)
  {
    NS_LOG_FUNCTION(this << app);
    m_app = app;
  }
  void
  HttpParser::ReadSocket(Ptr<Socket> socket)
  {
    NS_LOG_FUNCTION(this << socket);
    Address from;
    int bytes = socket->RecvFrom(&m_buffer[m_bytes], MPEG_MAX_MESSAGE - m_bytes,
        0, from);

    MPEGHeader mpeg_header;
    HTTPHeader http_header;

    uint32_t headersize = mpeg_header.GetSerializedSize()
        + http_header.GetSerializedSize();

    NS_LOG_INFO("Bytes = " << bytes);
    if (bytes > 0)
      {
        m_bytes += bytes;

        if (m_lastmeasurement > Time("0s"))
          {
            NS_LOG_INFO(
                Simulator::Now().GetSeconds() << " bytes: " << bytes << " dt: " << (Simulator::Now() - m_lastmeasurement).GetSeconds() << " bitrate: " << (8 * (bytes + headersize)/ (Simulator::Now() - m_lastmeasurement).GetSeconds()));
          }
        m_lastmeasurement = Simulator::Now();
      }

    NS_LOG_INFO(
        "### Buffer space: " << m_bytes << " Queue length " << m_app->GetPlayer().GetQueueSize());

    if (m_bytes < headersize)
      {
        return;
      }

    NS_LOG_INFO("hello world1");

    Packet headerPacket(m_buffer, headersize);
    headerPacket.RemoveHeader(mpeg_header);

    NS_LOG_INFO("hello world2");

    uint32_t message_size = headersize + mpeg_header.GetSize();

    NS_LOG_INFO("hello world3, m_byes="<< m_bytes<<" message_size="<< message_size );

    if (m_bytes < message_size)
      {
        return;
      }
    NS_LOG_INFO("hello world4");

    Packet message(m_buffer, message_size);

    NS_LOG_INFO("hello world5");

    memmove(m_buffer, &m_buffer[message_size], m_bytes - message_size);
    m_bytes -= message_size;

    NS_LOG_INFO("hello world6");

    m_app->MessageReceived(message);

    NS_LOG_INFO("hello world7");

    ReadSocket(socket);

    NS_LOG_INFO("hello world8");
  }
} // namespace ns3
