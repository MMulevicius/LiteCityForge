#pragma once
#include "Road/RoadNetwork.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>


namespace road
{

    struct CellKey
    {
        int x{};
        int y{};

        bool operator==(const CellKey& other) const {return x == other.x && y == other.y;}
    };

    struct CellKeyHash
    {
        std::size_t operator()(const CellKey& k) const noexcept
        {
            //simple hash combine
            std::size_t h1 = std::hash<int>{}(k.x);
            std::size_t h2 = std::hash<int>{}(k.y);
            return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
        }
    };

    class RoadQuery
    {
        private:

        RoadNetwork& mNet;
        float mCellSize;

        std::unordered_map<CellKey, std::vector<NodeId>, CellKeyHash> mNodeCells;
        std::unordered_map<CellKey, std::vector<SegId>, CellKeyHash> mSegCells;

        CellKey CellKeyFromPos(const glm::vec2& pos) const;
        void NeighborCellKeys(const CellKey& center, std::vector<CellKey>& out) const;

        void AABBForSegment(const glm::vec2& a, const glm::vec2& b, glm::vec2& outMin, glm::vec2& outMax) const;
        void CellsOverlappingAABB(const glm::vec2& mn, const glm::vec2& mx, std::vector<CellKey>& outKeys) const;

        public:

        RoadQuery(RoadNetwork& net, float cellSize);

        void InsertNode(NodeId nodeId);
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