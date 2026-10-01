#pragma once
#include "Road/RoadNetwork.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>


namespace road
{

    //grid cell key for spatial hashing
    struct CellKey
    {
        int x{};
        int y{};
        //for unordered_map key comparison
        bool operator==(const CellKey& other) const {return x == other.x && y == other.y;}
    };

    //hash function for cellkey
    struct CellKeyHash
    {
        std::size_t operator()(const CellKey& k) const noexcept
        {
            //hash combine
            std::size_t h1 {std::hash<int>{}(k.x)};
            std::size_t h2 {std::hash<int>{}(k.y)};
            return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
        }
    };

    
    class RoadQuery
    {
        private:

        //road graph and world-space size of one grid cell
        RoadNetwork& mNet;
        float mCellSize;

        //spatial hash: cell -> nodes in that cell
        std::unordered_map<CellKey, std::vector<NodeId>, CellKeyHash> mNodeCells;
        //spatial hash: cell -> segments overlapping that cell
        std::unordered_map<CellKey, std::vector<SegId>, CellKeyHash> mSegCells;

        //converts world position into grid cell coordinates
        CellKey CellKeyFromPos(const glm::vec2& pos) const;
        //returns the 3x3 neighbox cell keys around a center cell
        void NeighborCellKeys(const CellKey& center, std::vector<CellKey>& out) const;
        //computes 2D AABB for a segment 
        void AABBForSegment(const glm::vec2& a, const glm::vec2& b, glm::vec2& outMin, glm::vec2& outMax) const;
        //collect all grid cells that overlap a gien AABB
        void CellsOverlappingAABB(const glm::vec2& mn, const glm::vec2& mx, std::vector<CellKey>& outKeys) const;

        public:
        //creates a query structure for "net" with the given grid cell size
        RoadQuery(RoadNetwork& net, float cellSize);
        //inserts a node into the spatial hash
        void InsertNode(NodeId nodeId);
        //inserts a segment into all overlapping cells
        void InsertSegment(SegId segId);

        //collect nearby nodes from 3x3 neighbor cells, then distance-filter 
        std::vector<NodeId> QueryNearbyNodes(const glm::vec2& pos, float radius) const;

        //snap-to-node helper
        bool FindNearestNode(const glm::vec2& pos, float snapRadius, NodeId ignoreNodeId,
                                NodeId& outNodeId, float& outDist) const;

        //fetch candidate segments by overlapping AABB cells
        std::vector<SegId> QueryNearbySegments(const glm::vec2& a, const glm::vec2& b) const;
    };





}
