#pragma once 
#include <vector>
#include <glm/glm.hpp>
#include "RoadTypes.h"

namespace road
{
    // container + manager for road graph data structure 
    class RoadNetwork
    {
        private:
            std::vector<Node> mNodes;
            std::vector<Segment> mSegments;
            NodeId mNextNodeId = 1;
            SegId mNextSegId = 1;


        public:

            NodeId AddNode(const glm::vec2 &p);
            SegId AddSegment(NodeId a, NodeId b, RoadType type);

            //getters
            const std::vector<Node>& Nodes() const {return mNodes;}
            const std::vector<Segment>& Segments() const { return mSegments;}

            void Clear();
    };
}