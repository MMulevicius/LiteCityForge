#include "Road/BlockExtractor.h"
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <algorithm>


namespace road 
{
    struct DirectedEdgeKey
    {
        NodeId from;
        NodeId to;

        bool operator==(const DirectedEdgeKey& o) const
        {
            return from == o.from && to == o.to;
        }
    };

    struct DirectedEdgeKeyHash
    {
        std::size_t operator()(const DirectedEdgeKey& k) const noexcept
        {
            return (std::size_t(k.from) << 32) ^ std::size_t(k.to);
        }
    };

    // static float Cross(const glm::vec2& a, const glm::vec2& b)
    // {
    //     return a.x * b.y - a.y * b.x;
    // }

    // static float Dot(const glm::vec2& a, const glm::vec2& b)
    // {
    //     return a.x * b.x + a.y * b.y;
    // }

    // static glm::vec2 NormalizeSafe(const glm::vec2& v)
    // {
    //     float len = std::sqrt(v.x * v.x + v.y * v.y);
    //     if (len <= 1e-6f) return glm::vec2(0, 0);
    //     return v / len;
    // }

    static float SignedArea(const std::vector<glm::vec2>& poly)
    {
        if (poly.size() < 3) return 0.0f;
        double a = 0.0;
        for (size_t i = 0; i < poly.size(); i++)
        {
            const glm::vec2& p = poly[i];
            const glm::vec2& q = poly[(i + 1) % poly.size()];
            a += (double)p.x * (double)q.y - (double)q.x * (double)p.y;
        }
        return float(0.5 * a);
    }

    static bool AlmostSame(const glm::vec2& a, const glm::vec2 b, float eps = 1e-4f)
    {
        return glm::distance(a, b) < eps;
    }

    // //return angle of smallest left turn
    // static float LeftTurnAngle(const glm::vec2& dirIn, const glm::vec2& dirOut)
    // {
    //     glm::vec2 a = NormalizeSafe(dirIn);
    //     glm::vec2 b = NormalizeSafe(dirOut);

    //     float c = Cross(a, b);
    //     float d = Dot(a, b);

    //     float ang = std::atan2(c, d);

    //     if (ang < 0.0f) ang += 2.0f * 3.1415926535f;
    //     return ang;

    //}

    std::vector<Block> BlockExtractor::ExtractBlocks(const RoadNetwork& net) const
    {
        std::vector<Block> blocks;

        const auto& nodes = net.Nodes();
        const auto& segs = net.Segments();

        if(nodes.empty() || segs.empty())
            return blocks;
        
        auto nodePos = [&](NodeId id) -> glm::vec2
        {
            return nodes.at(id - 1).pos;
        };

        std::vector<std::vector<NodeId>> adj(nodes.size() + 1);
        adj.shrink_to_fit();

        //build adjacency
        for (const auto& s: segs)
        {
            if(s.a == 0 || s.b == 0) continue;
            adj[s.a].push_back(s.b);
            adj[s.b].push_back(s.a);
        }

        //sort each node's neighbor list
        for (NodeId v = 1; v < adj.size(); v++)
        {
            auto& nbrs = adj[v];
            if (nbrs.size() < 2) continue;

            const glm::vec2 pv = nodePos(v);

            std::sort(nbrs.begin(), nbrs.end(), [&](NodeId a, NodeId b)
            {
                const glm::vec2 pa = nodePos(a) - pv;
                const glm::vec2 pb = nodePos(b) - pv;

                float aa = std::atan2(pa.y, pa.x);
                float ab = std::atan2(pb.y, pb.x);
                return aa < ab;
            });

            nbrs.erase(std::unique(nbrs.begin(), nbrs.end()), nbrs.end());

        }

        auto findIndex = [&](const std::vector<NodeId>& v, NodeId x) -> int
        {
            for (int i = 0; i < (int)v.size(); i++)
                if (v[i] == x) return i;
            return -1;
        };

        auto nextEdge = [&](NodeId u, NodeId v) -> DirectedEdgeKey
        {  
            const auto& nbrs = adj[v];
            if (nbrs.empty()) return {v, 0};

            int idx = findIndex(nbrs, u);
            if (idx < 0)
            {
                return {v, nbrs[0]};
            }

            int nextIdx = (idx - 1);
            if (nextIdx < 0) nextIdx = (int)nbrs.size() - 1;


            return {v, nbrs[nextIdx]};

        };

        std::unordered_set<DirectedEdgeKey, DirectedEdgeKeyHash> used;

        for (const auto& s : segs)
        {
            DirectedEdgeKey starts[2] = { {s.a, s.b}, {s.b, s.a}};

            for (const auto& start : starts)
            {
                if (start.from == 0 || start.to == 0) continue;
                if (used.find(start) != used.end()) continue;

                std::vector<DirectedEdgeKey> walkedEdges;
                walkedEdges.reserve(256);

                std::vector<glm::vec2> loop;
                loop.reserve(256);

                DirectedEdgeKey cur = start;

                std::unordered_set<DirectedEdgeKey, DirectedEdgeKeyHash> localSeen;

                for (int step = 0; step < 8192; step++)
                {
                    if(cur.to == 0) break;

                    if(localSeen.find(cur) != localSeen.end())
                    {
                        break;
                    }

                    localSeen.insert(cur);

                    walkedEdges.push_back(cur);
                    loop.push_back(nodePos(cur.from));

                    DirectedEdgeKey nxt = nextEdge(cur.from, cur.to);
                    if(nxt.to == 0) break;

                    if(nxt.from == start.from && nxt.to == start.to)
                    {
                        loop.push_back(nodePos(cur.to));

                        if (!loop.empty() && AlmostSame(loop.front(), loop.back()))
                            loop.pop_back();

                        if(loop.size() >= 3)
                        {
                            float a = SignedArea(loop);

                            if(std::fabs(a) >= minBlockArea)
                            {
                                Block b;
                                b.id = (BlockId)blocks.size() + 1;
                                b.boundary = loop;
                                b.area = a;
                                blocks.push_back(std::move(b));

                                for(const auto& e : walkedEdges)
                                    used.insert(e);
                            }
                        }
                        break;
                    }
                    cur = nxt;

                }
            }

        }

        if (!blocks.empty())
        {
            auto it = std::max_element(blocks.begin(), blocks.end(), [](const Block& a, const Block& b)
            {
                return std::fabs(a.area) < std::fabs(b.area);
            });

            blocks.erase(it);

        }

        return blocks;

    }
}