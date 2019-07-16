#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"

#include <map>
#include <queue>

enum NodeType {
    User, Fog, Hybrid
};

typedef struct VideoStreamDash_h
{
    int clientIndex;
    int fogIndex;
    Ptr<Socket> client;
    Ptr<Socket> fognode;
} VideoStreamDash;

// typedef map<unsigned int, Segment> VideoSegments;
// typedef map<const char*, VideoSegments> VideoStreamMap;
