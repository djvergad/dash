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
#include "mpeg-header.h"

NS_LOG_COMPONENT_DEFINE("MPEGHeader");

namespace ns3
{

  NS_OBJECT_ENSURE_REGISTERED (MPEGHeader)
  ;

  MPEGHeader::MPEGHeader() :
      m_seq(0), m_ts(Simulator::Now().GetTimeStep()), m_frame_id(0), m_playback_time(
          0), m_type('B')

  {
    NS_LOG_FUNCTION(this);
  }

  void
  MPEGHeader::SetSeq(uint32_t seq)
  {
    NS_LOG_FUNCTION(this << seq);
    m_seq = seq;
  }
  uint32_t
  MPEGHeader::GetSeq(void) const
  {
    NS_LOG_FUNCTION(this);
    return m_seq;
  }

  void
  MPEGHeader::SetFrameId(uint32_t frame_id)
  {
    NS_LOG_FUNCTION(this);
    m_frame_id = frame_id;
  }
  uint32_t
  MPEGHeader::GetFrameId(void) const
  {
    NS_LOG_FUNCTION(this);
    return m_frame_id;
  }

  void
  MPEGHeader::SetPlaybackTime(Time playback_time)
  {
    NS_LOG_FUNCTION(this);
    m_playback_time = playback_time.GetTimeStep();
  }
  Time
  MPEGHeader::GetPlaybackTime(void) const
  {
    NS_LOG_FUNCTION(this);
    return TimeStep(m_playback_time);
  }

  void
  MPEGHeader::SetType(uint32_t type)
  {
    NS_LOG_FUNCTION(this);
    m_type = type;
  }
  uint32_t
  MPEGHeader::GetType(void) const
  {
    NS_LOG_FUNCTION(this);
    return m_type;
  }

  void
  MPEGHeader::SetSize(uint32_t size)
  {
    NS_LOG_FUNCTION(this);
    m_size = size;
  }

  uint32_t
  MPEGHeader::GetSize(void) const
  {
    NS_LOG_FUNCTION(this);
    return m_size;
  }
  Time
  MPEGHeader::GetTs(void) const
  {
    NS_LOG_FUNCTION(this);
    return TimeStep(m_ts);
  }

  TypeId
  MPEGHeader::GetTypeId(void)
  {
    static TypeId tid =
        TypeId("ns3::MPEGHeader").SetParent<Header>().AddConstructor<MPEGHeader>();
    return tid;
  }
  TypeId
  MPEGHeader::GetInstanceTypeId(void) const
  {
    return GetTypeId();
  }
  void
  MPEGHeader::Print(std::ostream &os) const
  {
    NS_LOG_FUNCTION(this << &os);
    os << "(seq=" << m_seq << " time=" << TimeStep(m_ts).GetSeconds() << ")";
  }
  uint32_t
  MPEGHeader::GetSerializedSize(void) const
  {
    NS_LOG_FUNCTION(this);
    return 4 + 8 + 4 + 8 + 4 + 4;
  }

  void
  MPEGHeader::Serialize(Buffer::Iterator start) const
  {
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    i.WriteHtonU32(m_seq);
    i.WriteHtonU64(m_ts);
    i.WriteHtonU32(m_frame_id);
    i.WriteHtonU64(m_playback_time);
    i.WriteHtonU32(m_type);
    i.WriteHtonU32(m_size);
  }
  uint32_t
  MPEGHeader::Deserialize(Buffer::Iterator start)
  {
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    m_seq = i.ReadNtohU32();
    m_ts = i.ReadNtohU64();
    m_frame_id = i.ReadNtohU32();
    m_playback_time = i.ReadNtohU64();
    m_type = i.ReadNtohU32();
    m_size = i.ReadNtohU32();
    return GetSerializedSize();
  }

} // namespace ns3
