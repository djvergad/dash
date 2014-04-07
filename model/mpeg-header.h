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

#ifndef MPEG_HEADER_H
#define MPEG_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"

namespace ns3
{
  /**
   * \ingroup udpclientserver
   * \class HTTPTsHeader
   * \brief Packet header for Udp client/server application
   * The header is made of a 32bits sequence number followed by
   * a 64bits time stamp.
   */

#define MPEG_MAX_MESSAGE 100000
#define MPEG_FRAMES_PER_SEGMENT 100
#define TIME_BETWEEN_FRAMES 20 // Miliseconds or 50 fps
  class MPEGHeader : public Header
  {
  public:
    MPEGHeader();

    /**
     * \param seq the sequence number
     */
    void
    SetSeq(uint32_t seq);
    /**
     * \return the sequence number
     */
    uint32_t
    GetSeq(void) const;
    /**
     * \return the time stamp
     */

    void
    SetFrameId(uint32_t frame_id);
    uint32_t
    GetFrameId(void) const;

    void
    SetPlaybackTime(Time playback_time);
    Time
    GetPlaybackTime(void) const;

    void
    SetType(uint32_t type);
    uint32_t
    GetType(void) const;

    void
    SetSize(uint32_t size);
    uint32_t
    GetSize(void) const;

    Time
    GetTs(void) const;

    static TypeId
    GetTypeId(void);

    virtual uint32_t
    GetSerializedSize(void) const;

  private:
    virtual TypeId
    GetInstanceTypeId(void) const;
    virtual void
    Print(std::ostream &os) const;
    virtual void
    Serialize(Buffer::Iterator start) const;
    virtual uint32_t
    Deserialize(Buffer::Iterator start);

    uint32_t m_seq;
    uint64_t m_ts;

    uint32_t m_frame_id;
    uint64_t m_playback_time;
    uint32_t m_type;
    uint32_t m_size;
  };

} // namespace ns3

#endif /* MPEG_HEADER_H */
