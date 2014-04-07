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

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "http-header.h"

NS_LOG_COMPONENT_DEFINE("HTTPHeader");

namespace ns3
{

  NS_OBJECT_ENSURE_REGISTERED (HTTPHeader)
  ;

  HTTPHeader::HTTPHeader() :
      m_seq(0), m_ts(Simulator::Now().GetTimeStep()), m_message_type(
          HTTP_REQUEST), m_video_id(0), m_resolution(0), m_segment_id(0)

  {
    NS_LOG_FUNCTION(this);
  }

  void
  HTTPHeader::SetSeq(uint32_t seq)
  {
    NS_LOG_FUNCTION(this << seq);
    m_seq = seq;
  }
  uint32_t
  HTTPHeader::GetSeq(void) const
  {
    NS_LOG_FUNCTION(this);
    return m_seq;
  }

  void
  HTTPHeader::SetMessageType(uint32_t message_type)
  {
    NS_LOG_FUNCTION(this << message_type);
    m_message_type = message_type;
  }

  uint32_t
  HTTPHeader::GetMessageType(void) const
  {
    NS_LOG_FUNCTION(this);
    return m_message_type;
  }
  void
  HTTPHeader::SetVideoId(uint32_t video_id)
  {

    NS_LOG_FUNCTION(this << video_id);
    m_video_id = video_id;
  }
  uint32_t
  HTTPHeader::GetVideoId(void) const
  {

    NS_LOG_FUNCTION(this);
    return m_video_id;
  }
  void
  HTTPHeader::SetResolution(uint32_t resolution)
  {

    NS_LOG_FUNCTION(this << resolution);
    m_resolution = resolution;
  }
  uint32_t
  HTTPHeader::GetResolution(void) const
  {
    NS_LOG_FUNCTION(this);
    return m_resolution;

  }
  void
  HTTPHeader::SetSegmentId(uint32_t segment_id)
  {
    NS_LOG_FUNCTION(this << segment_id);
    m_segment_id = segment_id;
  }
  uint32_t
  HTTPHeader::GetSegmentId(void) const
  {
    NS_LOG_FUNCTION(this);
    return m_segment_id;
  }

  Time
  HTTPHeader::GetTs(void) const
  {
    NS_LOG_FUNCTION(this);
    return TimeStep(m_ts);
  }

  TypeId
  HTTPHeader::GetTypeId(void)
  {
    static TypeId tid =
        TypeId("ns3::HTTPHeader").SetParent<Header>().AddConstructor<HTTPHeader>();
    return tid;
  }
  TypeId
  HTTPHeader::GetInstanceTypeId(void) const
  {
    return GetTypeId();
  }
  void
  HTTPHeader::Print(std::ostream &os) const
  {
    NS_LOG_FUNCTION(this << &os);
    os << "(seq=" << m_seq << " time=" << TimeStep(m_ts).GetSeconds() << ")";
  }
  uint32_t
  HTTPHeader::GetSerializedSize(void) const
  {
    NS_LOG_FUNCTION(this);
    return 4 + 8 + 4 + 4 + 4 + 4;
  }

  void
  HTTPHeader::Serialize(Buffer::Iterator start) const
  {
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    i.WriteHtonU32(m_seq);
    i.WriteHtonU64(m_ts);
    i.WriteHtonU32(m_message_type);
    i.WriteHtonU32(m_video_id);
    i.WriteHtonU32(m_resolution);
    i.WriteHtonU32(m_segment_id);
  }
  uint32_t
  HTTPHeader::Deserialize(Buffer::Iterator start)
  {
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    m_seq = i.ReadNtohU32();
    m_ts = i.ReadNtohU64();
    m_message_type = i.ReadNtohU32();
    m_video_id = i.ReadNtohU32();
    m_resolution = i.ReadNtohU32();
    m_segment_id = i.ReadNtohU32();
    return GetSerializedSize();
  }

} // namespace ns3
