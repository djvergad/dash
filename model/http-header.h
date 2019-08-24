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

#ifndef HTTP_TS_HEADER_H
#define HTTP_TS_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"

namespace ns3 {
/**
   * \ingroup dash
   * \class HTTPTHeader
   * \brief Packet header for HTTP layer of the DASH protocol
   */

#define HTTP_REQUEST 0
#define HTTP_RESPONSE 1

class HTTPHeader : public Header
{
public:
  HTTPHeader ();

  /**
     * \param seq the sequence number
     */
  void SetSeq (uint32_t seq);

  /**
     * \return the sequence number
     */
  uint32_t GetSeq (void) const;

  /**
     * \return the time stamp
     */
  void SetMessageType (uint32_t message_type);
  uint32_t GetMessageType (void) const;

  void SetVideoId (uint32_t video_id);
  uint32_t GetVideoId (void) const;

  void SetResolution (uint32_t resolution);
  uint32_t GetResolution (void) const;

  void SetSegmentId (uint32_t segment_id);
  uint32_t GetSegmentId (void) const;

  Time GetTs (void) const;

  static TypeId GetTypeId (void);

  virtual uint32_t GetSerializedSize (void) const;

private:
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  uint32_t m_seq;
  uint64_t m_ts;

  uint32_t m_message_type;
  uint32_t m_video_id;
  uint32_t m_resolution;
  uint32_t m_segment_id;
};

} // namespace ns3

#endif /* HTTP_TS_HEADER_H */
