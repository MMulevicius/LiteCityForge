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

    struct LotCollection;
    struct Block;

    void BuildRoadLineVerts(const RoadNetwork& net, std::vector<glm::vec3>& outHighways,
                                    std::vector<glm::vec3>& outStreets, float y = 0.05f);
    void BuildLotLineVerts(const LotCollection& lots, std::vector<glm::vec3>& outLots, float y = 0.02f);
    void BuildGardenLineVerts(const LotCollection& lots, std::vector<glm::vec3>& outGardens, float y = 0.021f);
    void BuildFootprintLineVerts(const LotCollection& lots, std::vector<glm::vec3>& outFootprints, float y = 0.022f);
        

    void BuildBlockOutlineVerts(const std::vector<Block>& blocks, std::vector<glm::vec3>& out, float y = 0.03f);
    void BuildBlockCentroidsVerts(const std::vector<Block>& blocks, std::vector<glm::vec3>& out, float y = 0.10f, float halfSize = 0.6f);
    int CountUnsplitIntersections(const RoadNetwork& net);
}