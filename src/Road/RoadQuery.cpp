#include "Road/RoadQuery.h"
#include <algorithm>
#include <cmath>


namespace road 
{
    static float Clamp01(float x)
    {
        if (x < 0.0f) return 0.0f;
        if (x > 1.0f) return 1.0f;
        return x;
    }

    RoadQuery::RoadQuery(RoadNetwork& net, float cellSize)
        : mNet(net), mCellSize(cellSize)
    {}

    void RoadQuery::Build()
    {
        mNodeCells.clear();
        mSegCells.clear();

        for(const auto& n: mNet.Nodes())
        {
            InsertNode(n.id);
        }
        for (const auto& s: mNet.Segments())
        {
            InsertSegment(s.id);
        }
    }

    void RoadQuery::InsertNode(NodeId nodeId)
    {
        const glm::vec2 pos = mNet.Nodes()[nodeId - 1].pos;
        CellKey k = CellKeyFromPos(pos);
        mNodeCells[k].push_back(nodeId);
    }

    void RoadQuery::InsertSegment(SegId segId)
    {
        const auto& seg = mNet.Segments()[segId - 1];
        
        const glm::vec2 a = mNet.Nodes()[seg.a - 1].pos;
        const glm::vec2 b = mNet.Nodes()[seg.b - 1].pos;

        glm::vec2 mn, mx;
        AABBForSegment(a, b, mn, mx);

        std::vector<CellKey> keys;
        CellsOverlappingAABB(mn, mx, keys);

        for (const auto& k: keys)
        {
            mSegCells[k].push_back(segId);
        }
    }

    std::vector<NodeId> RoadQuery::QueryNearbyNodes(const glm::vec2& pos, float radius) const
    {
        std::vector<CellKey> neigh;
        neigh.reserve(9);
        NeighborCellKeys(CellKeyFromPos(pos), neigh);

        std::vector<NodeId> candidates;
        for (const auto& k : neigh)
        {
            auto it = mNodeCells.find(k);
            if (it == mNodeCells.end()) continue;
            candidates.insert(candidates.end(), it->second.begin(), it->second.end());
        }

        //distance filter + de-dup
        std::sort(candidates.begin(), candidates.end());
        candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
        
        std::vector<NodeId> result;
        const float r2 = radius * radius;
        for (NodeId id: candidates)
        {
            const glm::vec2 np = mNet.Nodes()[id - 1].pos;
            if (glm::dot(np - pos, np - pos) <= r2)
                result.push_back(id);
        }
        return result;

    }

    bool RoadQuery::FindNearestNode(const glm::vec2& pos, float snapRadius, NodeId ignoreNodeId,
                                        NodeId& outNodeId, float& outDist) const
    {
        auto near = QueryNearbyNodes(pos, snapRadius);

        bool found = false;
        float best = 1e30f;
        NodeId bestId = 0;

        for (NodeId id : near)
        {
            if (id == ignoreNodeId) continue;
            const glm::vec2 np = mNet.Nodes()[id - 1].pos;
            float d = glm::length(np - pos);
            if (d < best)
            {
                best = d;
                bestId = id;
                found = true;
            }
        }

        if (found)
        {
            outNodeId = bestId;
            outDist = best;
        }
        return found;
    }

    std::vector<SegId> RoadQuery::QueryNearbySegments(const glm::vec2& a, const glm::vec2& b) const
    {
        glm::vec2 mn, mx;
        AABBForSegment(a, b, mn, mx);

        std::vector<CellKey> keys;
        CellsOverlappingAABB(mn, mx, keys);

        std::vector<SegId> temp;
        for (const auto& k : keys)
        {
            auto it = mSegCells.find(k);
            if (it == mSegCells.end()) continue;
            temp.insert(temp.end(), it->second.begin(), it->second.end());
        }

        std::sort(temp.begin(), temp.end());
        temp.erase(std::unique(temp.begin(), temp.end()), temp.end());
        return temp;
    }

    CellKey RoadQuery::CellKeyFromPos(const glm::vec2& pos) const
    {
        return CellKey {
            (int)std::floor(pos.x / mCellSize),
            (int)std::floor(pos.y / mCellSize)
        };
    }

    void RoadQuery::NeighborCellKeys(const CellKey& center, std::vector<CellKey>& out) const
    {
        out.clear();
        out.reserve(9);
        for (int dx = -1; dx <= 1; dx++)
            for(int dy = -1; dy <= 1; ++dy)
                out.push_back(CellKey{center.x + dx, center.y + dy});
    }

    void RoadQuery::AABBForSegment(const glm::vec2& a, const glm::vec2& b, glm::vec2& outMin, glm::vec2& outMax) const
    {
        outMin = { std::min(a.x, b.x), std::min(a.y, b.y) };
        outMax = { std::max(a.x, b.x), std::max(a.y, b.y) };
    }

    void RoadQuery::CellsOverlappingAABB(const glm::vec2& mn, const glm::vec2& mx, std::vector<CellKey>& outKeys) const
    {
        outKeys.clear();

        int x0 = (int)std::floor(mn.x / mCellSize);
        int x1 = (int)std::floor(mx.x / mCellSize);
        int y0 = (int)std::floor(mn.y / mCellSize);
        int y1 = (int)std::floor(mx.y / mCellSize);

        for (int x = x0; x <= x1; x++)
            for (int y = y0; y <= y1; y++)
                outKeys.push_back(CellKey{ x, y});
    }
}