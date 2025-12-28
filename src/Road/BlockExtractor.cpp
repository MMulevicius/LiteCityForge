#include "Road/BlockExtractor.h"
#include <unordered_map>
#include <unordered_set>
#include <iostream>
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

    static void RemoveConsecutiveDuplicates(std::vector<glm::vec2>& poly, float eps = 1e-4f)
    {
        if (poly.size() < 2) return;

        std::vector<glm::vec2> out;
        out.reserve(poly.size());

        out.push_back(poly[0]);
        for (size_t i = 1; i < poly.size(); i++)
        {
            if (glm::distance(out.back(), poly[i]) >= eps)
                out.push_back(poly[i]);
        }

        if (out.size() >= 2 && glm::distance(out.front(), out.back()) < eps)
            out.pop_back();

            poly.swap(out);
    }

    static void ComputeAABB(const std::vector<glm::vec2>& poly, glm::vec2& mn, glm::vec2& mx)
    {
        mn = glm::vec2(1e30f);
        mx = glm::vec2(-1e30f);
        for(const auto& p : poly)
        {
            mn.x = std::min(mn.x, p.x); 
            mn.y = std::min(mn.y, p.y);
            mx.x = std::max(mx.x, p.x); 
            mx.y = std::max(mx.y, p.y);
        }
    }

    static glm::vec2 Centroid(const std::vector<glm::vec2>& poly)
    {
        if(poly.empty()) return glm::vec2(0.0f);
        glm::vec2 c(0.0f);
        for (const auto& p : poly) c += p;
        return c / (float)poly.size();
    }

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
        //adj.shrink_to_fit();

        //build adjacency
        for (const auto& s: segs)
        {
            if(s.a == 0 || s.b == 0) continue;

            if(s.type == RoadType::Highway)
                continue;

            adj[s.a].push_back(s.b);
            adj[s.b].push_back(s.a);
        }

        {
            std::vector<Block> unique;
            unique.reserve(blocks.size());

            for (const auto& b : blocks)
            {
                glm::vec2 cb = Centroid(b.boundary);

                bool dup = false;
                for(const auto& u : unique)
                {
                    glm::vec2 cu = Centroid(u.boundary);

                    if (glm::distance(cb, cu) < 1.0f)
                    {
                        dup = true;
                        break;
                    }
                }

                if(!dup)
                    unique.push_back(b);
            }
            blocks.swap(unique);

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

            // int nextIdx = (idx + 1);
            // if (nextIdx >= (int)nbrs.size()) nextIdx = 0;
            int nextIdx = (idx - 1);
            if (nextIdx < 0) nextIdx = (int)nbrs.size() - 1;


            return {v, nbrs[nextIdx]};

        };

        std::unordered_set<DirectedEdgeKey, DirectedEdgeKeyHash> used;

        for (const auto& s : segs)
        {
            if (s.type == RoadType::Highway)
                continue;

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

                        RemoveConsecutiveDuplicates(loop);

                        if(loop.size() >= 4)
                        {
                            float a = SignedArea(loop);

                            if(std::fabs(a) >= minBlockArea)
                            {
                                glm::vec2 mn, mx;
                                ComputeAABB(loop, mn, mx);

                                float w = mx.x - mn.x;
                                float h = mx.y - mn.y;

                                //reject tiny or poor quality blocks
                                if (w < 2.0f || h < 2.0f)
                                    break;

                                float aspect = (w > h) ? (w / h) : (h / w);
                                if (aspect > 6.0f)
                                    break;

                                Block b;
                                b.id = (BlockId)blocks.size() + 1;
                                b.boundary = loop;
                                b.area = a;
                                blocks.push_back(std::move(b));
                                std::cout << "block area: " << std::fabs(a) << "  verts:" << loop.size() << "\n";


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

        if (blocks.size() >= 2)
        {
            std::sort(blocks.begin(), blocks.end(),
                [](const Block& a, const Block& b) {return std::fabs(a.area) > std::fabs(b.area);});

            float a0 = std::fabs(blocks[0].area);
            float a1 = std::fabs(blocks[1].area);

            if(a0 > 3.0f * a1)
                blocks.erase(blocks.begin());
        }

        return blocks;

    }
}