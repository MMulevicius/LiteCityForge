#include "Road/RoadNetwork.h"

namespace road
{

    NodeId RoadNetwork::AddNode(const glm::vec2 &p)
    {
        Node n;
        n.id = mNextNodeId++;
        n.pos = p;
        mNodes.push_back(n);
        return n.id;
    }

    SegId RoadNetwork::AddSegment(NodeId a, NodeId b, RoadType type)
    {
        Segment s;
        s.id = mNextSegId++;
        s.a = a;
        s.b = b;
        s.type = type;
        mSegments.push_back(s);
        return s.id;
    }

    void RoadNetwork::Clear()
    {
        mNodes.clear();
        mSegments.clear();
        mNextNodeId = 1;
        mNextSegId = 1;
    }
}