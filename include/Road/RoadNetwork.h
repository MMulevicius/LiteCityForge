#pragma once 
#include <vector>
#include <glm/glm.hpp>
#include "RoadTypes.h"

namespace road
{

    // container + manager for road graph data structure 
    // stores the procedural road graph
    class RoadNetwork
    {
        //intersection nodes + road segments
        // node ID and segment ID allocators
        private:
            std::vector<Node> mNodes;
            std::vector<Segment> mSegments;
            NodeId mNextNodeId = 1;
            SegId mNextSegId = 1;
              


        public:


            //add a new intersection
            NodeId AddNode(const glm::vec2 &p);
            //add a road segment between two nodes
            SegId AddSegment(NodeId a, NodeId b, RoadType type);

            //getters
            const std::vector<Node>& Nodes() const {return mNodes;}
            const std::vector<Segment>& Segments() const { return mSegments;}

            //clears the network and resets ID counters
            void Clear();


            
    };

    struct LotCollection;
    struct Block;

    //line vertices for debug 2D rendering of roads
    void BuildRoadLineVerts(const RoadNetwork& net, std::vector<glm::vec3>& outHighways,
                                    std::vector<glm::vec3>& outStreets, float y = 0.05f);
    //line vertices for lot boundaries
    void BuildLotLineVerts(const LotCollection& lots, std::vector<glm::vec3>& outLots, float y = 0.02f);
    //line vertices for garden outlines
    void BuildGardenLineVerts(const LotCollection& lots, std::vector<glm::vec3>& outGardens, float y = 0.021f);
    //line vertices for building footprints
    void BuildFootprintLineVerts(const LotCollection& lots, std::vector<glm::vec3>& outFootprints, float y = 0.022f);
    //line vertices for block outlines
    void BuildBlockOutlineVerts(const std::vector<Block>& blocks, std::vector<glm::vec3>& out, float y = 0.03f);
    //counts intersections that haven't been split yet
    int CountUnsplitIntersections(const RoadNetwork& net);
}
