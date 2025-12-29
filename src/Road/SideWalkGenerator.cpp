#include "Road/SideWalkGenerator.h"
#include "Road/RoadNetwork.h"
#include "Road/RoadParams.h"
#include "Road/RoadTypes.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace road
{
    static inline float Cross2(const glm::vec2& a, const glm::vec2& b)
    {
        return a.x * b.y - a.y * b.x;
    }

    static inline void AddLine(std::vector<glm::vec3>& v, const glm::vec3& a, const glm::vec3& b)
    {
        v.push_back(a);
        v.push_back(b);
    }

    static inline glm::vec2 Perp(const glm::vec2& d) { return glm::vec2(-d.y, d.x); }

    static inline glm::vec2 NormalizeSafe(const glm::vec2& v)
    {
        const float l2 = glm::dot(v, v);
        if (l2 < 1e-8f) return glm::vec2(0.0f);
        return v * (1.0f / std::sqrt(l2));
    }

    static inline float Abs(float x) { return x < 0.0f ? -x : x; }

    static bool IntersectLines(const glm::vec2& p0, const glm::vec2& d0,
                               const glm::vec2& p1, const glm::vec2& d1,
                               glm::vec2& outP)
    {
        const float denom = Cross2(d0, d1);
        if (Abs(denom) < 1e-6f) return false;
        const glm::vec2 r = p1 - p0;
        const float t = Cross2(r, d1) / denom;
        outP = p0 + d0 * t;
        return true;
    }

    static inline float SidewalkOffsetFor(const Segment& seg, const RoadParams& params)
    {
        const float halfW = (seg.type == RoadType::Highway) ? params.highwayHalfWidth : params.streetHalfWidth;
        return halfW + params.sidewalkGap + (params.sidewalkWidth * 0.5f);
    }

    static inline float SidewalkTrimFor(const Segment& seg, const RoadParams& params)
    {
        // trim from node so intersections (degree 3/4)
        const float halfW = (seg.type == RoadType::Highway) ? params.highwayHalfWidth : params.streetHalfWidth;
        return (halfW + params.sidewalkGap + params.sidewalkWidth) * 0.75f;
    }

    struct SegSW
    {
        glm::vec2 L0, L1; 
        glm::vec2 R0, R1; 
        glm::vec2 d;      
        glm::vec2 n;      
        float off = 0.0f; 
    };

    void BuildSidewalkLineVerts(const RoadNetwork& net, const RoadParams& params,
                                std::vector<glm::vec3>& outSidewalks, float y)
    {
        outSidewalks.clear();

        const auto& nodes = net.Nodes();
        const auto& segs  = net.Segments();

        if (segs.empty() || nodes.empty())
            return;

        outSidewalks.reserve(segs.size() * 6);

        // build degrees + incidence lists
        std::vector<int> degree(nodes.size() + 1, 0);
        std::vector<std::vector<int>> incident(nodes.size() + 1);

        incident.shrink_to_fit();
        incident.resize(nodes.size() + 1);

        for (int i = 0; i < (int)segs.size(); ++i)
        {
            const int a = (int)segs[i].a;
            const int b = (int)segs[i].b;
            if (a >= 1 && a <= (int)nodes.size()) { degree[a]++; incident[a].push_back(i); }
            if (b >= 1 && b <= (int)nodes.size()) { degree[b]++; incident[b].push_back(i); }
        }

        // precompute per-segment sidewalk endpoints (with trim except for degree==2 ends)
        std::vector<SegSW> sw(segs.size());

        for (int i = 0; i < (int)segs.size(); ++i)
        {
            const Segment& s = segs[i];

            const glm::vec2 A = nodes.at((size_t)s.a - 1).pos;
            const glm::vec2 B = nodes.at((size_t)s.b - 1).pos;

            glm::vec2 d = NormalizeSafe(B - A);
            if (glm::dot(d, d) < 1e-8f) continue;

            glm::vec2 n = NormalizeSafe(Perp(d));

            const float off  = SidewalkOffsetFor(s, params);
            const float trim = SidewalkTrimFor(s, params);

            // don’t trim at degree==2 endpoints
            const float trimA = (degree[(int)s.a] <= 2) ? 0.0f : trim;
            const float trimB = (degree[(int)s.b] <= 2) ? 0.0f : trim;

            const glm::vec2 A2 = A + d * trimA;
            const glm::vec2 B2 = B - d * trimB;

            // if a segment becomes too short after trim, skip it.
            if (glm::dot(B2 - A2, B2 - A2) < 1e-6f) continue;

            SegSW& out = sw[i];
            out.d   = d;
            out.n   = n;
            out.off = off;

            out.L0 = A2 + n * off;
            out.L1 = B2 + n * off;
            out.R0 = A2 - n * off;
            out.R1 = B2 - n * off;
        }

        // miter-join ONLY degree==2 nodes
        const float miterLimit = 6.0f; 

        for (int nodeId = 1; nodeId <= (int)nodes.size(); ++nodeId)
        {
            if (degree[nodeId] != 2) continue;
            const auto& inc = incident[nodeId];
            if (inc.size() != 2) continue;

            const int s0i = inc[0];
            const int s1i = inc[1];
            const Segment& s0 = segs[s0i];
            const Segment& s1 = segs[s1i];

            const glm::vec2 C = nodes.at((size_t)nodeId - 1).pos;

            // segment 0 geometry
            const glm::vec2 A0 = nodes.at((size_t)s0.a - 1).pos;
            const glm::vec2 B0 = nodes.at((size_t)s0.b - 1).pos;
            const glm::vec2 d0 = NormalizeSafe(B0 - A0);
            if (glm::dot(d0, d0) < 1e-8f) continue;
            const glm::vec2 n0 = NormalizeSafe(Perp(d0));
            const float off0    = SidewalkOffsetFor(s0, params);

            // segment 1 geometry
            const glm::vec2 A1 = nodes.at((size_t)s1.a - 1).pos;
            const glm::vec2 B1 = nodes.at((size_t)s1.b - 1).pos;
            const glm::vec2 d1 = NormalizeSafe(B1 - A1);
            if (glm::dot(d1, d1) < 1e-8f) continue;
            const glm::vec2 n1 = NormalizeSafe(Perp(d1));
            const float off1    = SidewalkOffsetFor(s1, params);

            // directions OUT of the node along each segment
            const glm::vec2 dir0 = ((int)s0.a == nodeId) ? d0 : -d0;
            const glm::vec2 dir1 = ((int)s1.a == nodeId) ? d1 : -d1;

            const float straightDot = glm::dot(dir0, dir1);
            // ~170°-180°
            if (straightDot < -0.985f) 
                continue;

            // two candidate offset vectors per segment at the node
            const glm::vec2 v0a =  n0 * off0;
            const glm::vec2 v0b = -n0 * off0;
            const glm::vec2 v1a =  n1 * off1;
            const glm::vec2 v1b = -n1 * off1;

            const float dAA = glm::dot(NormalizeSafe(v0a), NormalizeSafe(v1a));
            const float dAB = glm::dot(NormalizeSafe(v0a), NormalizeSafe(v1b));

            // if v0a matches v1a better => (v0a<->v1a) and (v0b<->v1b)
            // else => (v0a<->v1b) and (v0b<->v1a)
            glm::vec2 p0_1, p1_1, p0_2, p1_2; 
            glm::vec2 o0_1, o1_1, o0_2, o1_2; 

            if (dAA >= dAB)
            {
                o0_1 = v0a; o1_1 = v1a;
                o0_2 = v0b; o1_2 = v1b;
            }
            else
            {
                o0_1 = v0a; o1_1 = v1b;
                o0_2 = v0b; o1_2 = v1a;
            }

            // intersect the offset lines for each matched pair
            glm::vec2 M1, M2;
            bool ok1 = IntersectLines(C + o0_1, dir0, C + o1_1, dir1, M1);
            bool ok2 = IntersectLines(C + o0_2, dir0, C + o1_2, dir1, M2);

            // clamp crazy miters (acute angles)
            auto clampMiter = [&](glm::vec2& M, const glm::vec2& oA, const glm::vec2& oB, float offMax)
            {
                const float dist = std::sqrt(glm::dot(M - C, M - C));
                if (dist > offMax * miterLimit)
                {
                    // fallback: average of the two offset points 
                    M = C + (oA + oB) * 0.5f;
                }
            };

            const float offMax1 = std::max(std::sqrt(glm::dot(o0_1, o0_1)), std::sqrt(glm::dot(o1_1, o1_1)));
            const float offMax2 = std::max(std::sqrt(glm::dot(o0_2, o0_2)), std::sqrt(glm::dot(o1_2, o1_2)));

            if (ok1) clampMiter(M1, o0_1, o1_1, offMax1);
            if (ok2) clampMiter(M2, o0_2, o1_2, offMax2);

            // assign the miter points back into the correct endpoint slots of each segment.
            auto applyToSegmentAtNode = [&](int segIndex, const Segment& s, const glm::vec2& nSeg, float offSeg,
                                            const glm::vec2& oWanted, const glm::vec2& M)
            {
                // determine if the matched offset for this seg is its +n side (left) or -n side (right)
                const float sign = glm::dot(NormalizeSafe(oWanted), NormalizeSafe(nSeg)) >= 0.0f ? +1.0f : -1.0f;

                const bool atA = ((int)s.a == nodeId);
                SegSW& out = sw[segIndex];

                if (sign > 0.0f)
                {
                    // +n => Left side
                    if (atA) out.L0 = M;
                    else     out.L1 = M;
                }
                else
                {
                    // -n => Right side
                    if (atA) out.R0 = M;
                    else     out.R1 = M;
                }
            };

            if (ok1)
            {
                applyToSegmentAtNode(s0i, s0, n0, off0, o0_1, M1);
                applyToSegmentAtNode(s1i, s1, n1, off1, o1_1, M1);
            }
            if (ok2)
            {
                applyToSegmentAtNode(s0i, s0, n0, off0, o0_2, M2);
                applyToSegmentAtNode(s1i, s1, n1, off1, o1_2, M2);
            }
        }

        // emit segment sidewalk lines
        for (int i = 0; i < (int)segs.size(); ++i)
        {
            const SegSW& s = sw[i];

            // skip segments that never got valid geometry
            if (glm::dot(s.d, s.d) < 1e-8f) continue;

            AddLine(outSidewalks, glm::vec3(s.L0.x, y, s.L0.y), glm::vec3(s.L1.x, y, s.L1.y));
            AddLine(outSidewalks, glm::vec3(s.R0.x, y, s.R0.y), glm::vec3(s.R1.x, y, s.R1.y));
            
        }

        // T-junction (degree==3) bar joins
        auto EndAtNode = [&](int segIndex, const Segment& seg, int nodeId, bool leftSide) -> glm::vec2
        {
            const SegSW& s = sw[segIndex];
            const bool atA = ((int)seg.a == nodeId);
            return leftSide ? (atA ? s.L0 : s.L1) : (atA ? s.R0 : s.R1);
        };

        auto DirAwayFromNode = [&](const Segment& seg, int nodeId) -> glm::vec2
        {
            const glm::vec2 A = nodes.at((size_t)seg.a - 1).pos;
            const glm::vec2 B = nodes.at((size_t)seg.b - 1).pos;
            glm::vec2 d = NormalizeSafe(B - A);
            if (glm::dot(d, d) < 1e-8f) return glm::vec2(0.0f);
            return ((int)seg.a == nodeId) ? d : -d;
        };

        auto PickOnSide = [&](int segIndex, const Segment& seg, int nodeId,
                            const glm::vec2& C, const glm::vec2& nRef, float signWanted) -> glm::vec2
        {
            // pick whichever of (L,R) lies more strongly on the requested side of nRef
            glm::vec2 pL = EndAtNode(segIndex, seg, nodeId, true);
            glm::vec2 pR = EndAtNode(segIndex, seg, nodeId, false);

            float sL = glm::dot(pL - C, nRef);
            float sR = glm::dot(pR - C, nRef);

            bool okL = (signWanted > 0.0f) ? (sL >= 0.0f) : (sL <= 0.0f);
            bool okR = (signWanted > 0.0f) ? (sR >= 0.0f) : (sR <= 0.0f);

            if (okL && okR) return (Abs(sL) >= Abs(sR)) ? pL : pR;
            if (okL) return pL;
            if (okR) return pR;

            // fallback: if both are barely on the other side due to precision, just pick the closer-to-side one
            return (Abs(sL) >= Abs(sR)) ? pL : pR;
        };

        for (int nodeId = 1; nodeId <= (int)nodes.size(); ++nodeId)
        {
            if (degree[nodeId] != 3) continue;
            const auto& inc = incident[nodeId];
            if (inc.size() != 3) continue;

            // find the two most opposite directions (the "bar" of the T)
            int bestI = -1, bestJ = -1;
            float bestDot = 1.0f;

            for (int a = 0; a < 3; ++a)
            {
                for (int b = a + 1; b < 3; ++b)
                {
                    const int siA = inc[a];
                    const int siB = inc[b];
                    const Segment& sA = segs[siA];
                    const Segment& sB = segs[siB];

                    glm::vec2 da = DirAwayFromNode(sA, nodeId);
                    glm::vec2 db = DirAwayFromNode(sB, nodeId);
                    if (glm::dot(da, da) < 1e-8f || glm::dot(db, db) < 1e-8f) continue;

                    float dp = glm::dot(da, db); 
                    if (dp < bestDot)
                    {
                        bestDot = dp;
                        bestI = siA;
                        bestJ = siB;
                    }
                }
            }

            // must actually be a "T bar" (reasonably opposite)
            if (bestI < 0 || bestJ < 0) continue;
            if (bestDot > -0.55f) continue;

            const Segment& sI = segs[bestI];
            const Segment& sJ = segs[bestJ];

            const glm::vec2 C = nodes.at((size_t)nodeId - 1).pos;

            // reference normal: "up/down" sides for the bar.
            glm::vec2 barDir = DirAwayFromNode(sI, nodeId);
            if (glm::dot(barDir, barDir) < 1e-8f) continue;
            glm::vec2 nRef = NormalizeSafe(Perp(barDir));

            glm::vec2 topI = PickOnSide(bestI, sI, nodeId, C, nRef, +1.0f);
            glm::vec2 topJ = PickOnSide(bestJ, sJ, nodeId, C, nRef, +1.0f);
            AddLine(outSidewalks, glm::vec3(topI.x, y, topI.y), glm::vec3(topJ.x, y, topJ.y));

            glm::vec2 botI = PickOnSide(bestI, sI, nodeId, C, nRef, -1.0f);
            glm::vec2 botJ = PickOnSide(bestJ, sJ, nodeId, C, nRef, -1.0f);
            AddLine(outSidewalks, glm::vec3(botI.x, y, botI.y), glm::vec3(botJ.x, y, botJ.y));

            int stemIdx = -1;
            for (int k = 0; k < 3; ++k)
            {
                int si = inc[k];
                if (si != bestI && si != bestJ)
                {
                    stemIdx = si;
                    break;
                }
            }
            if (stemIdx >= 0)
            {
                const Segment& stemSeg = segs[stemIdx];
                glm::vec2 dirStem = DirAwayFromNode(stemSeg, nodeId);
                if (glm::dot(dirStem, dirStem) > 1e-8f)
                {
                    // clear distance should be based on the biggest road at this node (intersection "radius")
                    float maxHalfW = 0.0f;
                    for (int k = 0; k < 3; ++k)
                    {
                        const Segment& ss = segs[inc[k]];
                        const float hw = (ss.type == RoadType::Highway) ? params.highwayHalfWidth : params.streetHalfWidth;
                        if (hw > maxHalfW) maxHalfW = hw;
                    }

                    // this is the distance from node along the stem where sidewalks should start 
                    const float clear = (maxHalfW + params.sidewalkGap + params.sidewalkWidth) * 1.50f;

                    glm::vec2 nStem = sw[stemIdx].n;

                  
                    if (glm::dot(sw[stemIdx].d, dirStem) < 0.0f)
                        nStem = -nStem;

                    const float     offStem = sw[stemIdx].off;  
                    const bool atA          = ((int)stemSeg.a == nodeId);

                    if (atA)
                    {
                        sw[stemIdx].L0 = (C + dirStem * clear) + nStem * offStem;
                        sw[stemIdx].R0 = (C + dirStem * clear) - nStem * offStem;
                    }
                    else
                    {
                        sw[stemIdx].L1 = (C + dirStem * clear) + nStem * offStem;
                        sw[stemIdx].R1 = (C + dirStem * clear) - nStem * offStem;
                    }

                }
            }

        }

        



        // dead-end caps (degree==1)
        for (int nodeId = 1; nodeId <= (int)nodes.size(); ++nodeId)
        {
            if (degree[nodeId] != 1) continue;
            const auto& inc = incident[nodeId];
            if (inc.size() != 1) continue;

            const int si = inc[0];
            const Segment& seg = segs[si];

            const bool atA = ((int)seg.a == nodeId);
            const SegSW& s = sw[si];

            const glm::vec2 Pleft  = atA ? s.L0 : s.L1;
            const glm::vec2 Pright = atA ? s.R0 : s.R1;

            AddLine(outSidewalks,
                    glm::vec3(Pleft.x,  y, Pleft.y),
                    glm::vec3(Pright.x, y, Pright.y));
        }
    }

}
