#include "Road/LotSubdivision.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <unordered_map>

namespace road
{
    static glm::vec2 CentroidQuad(const std::vector<glm::vec2>& p)
    {
        return 0.25f * (p[0] + p[1] + p[2] + p[3]);
    }

    static float Clamp01(float x)
    {
        return std::max(0.0f, std::min(1.0f, x));
    }

    static float Lerp(float a, float b, float t)
    {
        return a + (b - a) * t;
    }

    static float Length(const glm::vec2& v)
    {
        return std::sqrt(v.x * v.x + v.y * v.y);
    }

    static glm::vec2 NormalizeSafe(const glm::vec2& v)
    {
        float len = Length(v);
        if (len <= 1e-6f) return glm::vec2(0.0f, 0.0f);
        return v / len;
    }

    static glm::vec2 LeftPerp(const glm::vec2& v)
    {
        // left normal
        return glm::vec2(-v.y, v.x);
    }

    static float SignedArea2D(const std::vector<glm::vec2>& poly)
    {
        // shoelace formula (signed)
        if (poly.size() < 3) return 0.0f;
        double a = 0.0;
        for (size_t i = 0; i < poly.size(); i++)
        {
            const glm::vec2& p = poly[i];
            const glm::vec2& q = poly[(i + 1) % poly.size()];
            a += (double)p.x * (double)q.y - (double)q.x * (double)p.y;
        }
        return (float)(0.5 * a);
    }


    static float AreaAbs(const std::vector<glm::vec2>&poly)
    {
        return std::fabs(SignedArea2D(poly));
    }

    static void EnsureCCW(std::vector<glm::vec2>& poly)
    {
        if (SignedArea2D(poly) < 0.0f)
            std::reverse(poly.begin(), poly.end());
    }

    static bool SegSegIntersect(const glm::vec2& a, const glm::vec2& b,
                            const glm::vec2& c, const glm::vec2& d)
    {
        auto cross = [](const glm::vec2& u, const glm::vec2& v) { return u.x*v.y - u.y*v.x; };
        glm::vec2 r = b - a;
        glm::vec2 s = d - c;
        float denom = cross(r, s);
        if (std::fabs(denom) < 1e-6f) return false; 

        glm::vec2 c_a = c - a;
        float t = cross(c_a, s) / denom;
        float u = cross(c_a, r) / denom;
        // allow a tiny amount of touching without counting as a collision
        const float eps = 1e-3f;
        return (t > eps && t < 1.f - eps && u > eps && u < 1.f - eps);

    }

    static bool LotIntersectsSegment(const std::vector<glm::vec2>& lot, const glm::vec2& s0, const glm::vec2& s1)
    {
        // check segment against each lot edge
        for (int i=0;i<4;i++)
        {
            const glm::vec2& a = lot[i];
            const glm::vec2& b = lot[(i+1)%4];
            if (SegSegIntersect(a, b, s0, s1))
                return true;
        }
        return false;
    }


    struct OBB2
    {
        glm::vec2 c;   
        glm::vec2 u0;  
        glm::vec2 u1;  
        float e0;      
        float e1;      

        glm::vec2 aabbMin;
        glm::vec2 aabbMax;
    };

    static OBB2 OBBFromLotPoly(const std::vector<glm::vec2>& poly, float padding)
    {
        // Expected order: near0, near1, far1, far0 (CCW)
        const glm::vec2& near0 = poly[0];
        const glm::vec2& near1 = poly[1];
        const glm::vec2& far1  = poly[2];
        const glm::vec2& far0  = poly[3];

        glm::vec2 edge0 = near1 - near0;
        glm::vec2 edge1 = far0 - near0;

        glm::vec2 u0 = NormalizeSafe(edge0);
        glm::vec2 u1 = NormalizeSafe(edge1);

        float e0 = 0.5f * Length(edge0) + padding;
        float e1 = 0.5f * Length(edge1) + padding;

        glm::vec2 c = 0.25f * (near0 + near1 + far0 + far1);

        // AABB from corners (broadphase)
        glm::vec2 mn = near0;
        glm::vec2 mx = near0;
        auto grow = [&](const glm::vec2& p)
        {
            mn.x = std::min(mn.x, p.x); mn.y = std::min(mn.y, p.y);
            mx.x = std::max(mx.x, p.x); mx.y = std::max(mx.y, p.y);
        };
        grow(near1); grow(far1); grow(far0);

        // Inflate AABB 
        mn -= glm::vec2(padding);
        mx += glm::vec2(padding);

        return OBB2{c, u0, u1, e0, e1, mn, mx};
    }

    static bool OBBOverlapSAT(const OBB2& A, const OBB2& B)
    {
        // SAT on 4 axes: A.u0, A.u1, B.u0, B.u1
        auto projRadius = [](const OBB2& O, const glm::vec2& axis)
        {
            return O.e0 * std::fabs(glm::dot(O.u0, axis)) +
                   O.e1 * std::fabs(glm::dot(O.u1, axis));
        };

        glm::vec2 t = B.c - A.c;

        auto sep = [&](const glm::vec2& axis)
        {
            if (Length(axis) <= 1e-6f) return false;
            glm::vec2 n = NormalizeSafe(axis);
            float dist = std::fabs(glm::dot(t, n));
            float r = projRadius(A, n) + projRadius(B, n);
            return dist > r;
        };

        if (sep(A.u0)) return false;
        if (sep(A.u1)) return false;
        if (sep(B.u0)) return false;
        if (sep(B.u1)) return false;
        return true;
    }

    struct CellKey
    {
        int x, y;
        bool operator==(const CellKey& o) const { return x == o.x && y == o.y; }
    };

    struct CellKeyHash
    {
        std::size_t operator()(const CellKey& k) const noexcept
        {
            std::size_t h1 = std::hash<int>{}(k.x);
            std::size_t h2 = std::hash<int>{}(k.y);
            return h1 * 1315423911u ^ (h2 + 0x9e3779b9u + (h1<<6) + (h1>>2));
        }
    };

    struct LotSpatialIndex
    {
        float cellSize = 6.0f;
        std::unordered_map<CellKey, std::vector<size_t>, CellKeyHash> cells;
        std::vector<OBB2> obbs;

        explicit LotSpatialIndex(float cs) : cellSize(std::max(cs, 0.1f)) {}

        CellKey cellOf(const glm::vec2& p) const
        {
            return CellKey{ (int)std::floor(p.x / cellSize), (int)std::floor(p.y / cellSize) };
        }

        void insert(const OBB2& o)
        {
            const size_t idx = obbs.size();
            obbs.push_back(o);

            CellKey c0 = cellOf(o.aabbMin);
            CellKey c1 = cellOf(o.aabbMax);
            for (int cy = c0.y; cy <= c1.y; ++cy)
                for (int cx = c0.x; cx <= c1.x; ++cx)
                    cells[CellKey{cx, cy}].push_back(idx);
        }

        bool overlapsAny(const OBB2& candidate) const
        {
            CellKey c0 = cellOf(candidate.aabbMin);
            CellKey c1 = cellOf(candidate.aabbMax);

            for (int cy = c0.y; cy <= c1.y; ++cy)
            {
                for (int cx = c0.x; cx <= c1.x; ++cx)
                {
                    auto it = cells.find(CellKey{cx, cy});
                    if (it == cells.end()) continue;

                    for (size_t idx : it->second)
                    {
                        const OBB2& o = obbs[idx];

                        // AABB quick reject
                        if (candidate.aabbMax.x < o.aabbMin.x || candidate.aabbMin.x > o.aabbMax.x ||
                            candidate.aabbMax.y < o.aabbMin.y || candidate.aabbMin.y > o.aabbMax.y)
                            continue;

                        if (OBBOverlapSAT(candidate, o))
                            return true;
                    }
                }
            }
            return false;
        }
    };

    static float RoadHalfWidth(RoadType type, const LotParams& p)
    {
        return (type == RoadType::Street) ? p.streetHalfWidth : p.highwayHalfWidth;
    }

    bool LotSubdivision::IsLotRoadType(RoadType t, const LotParams& params)
    {
        if (t == RoadType::Street && params.placeLotsOnStreets) return true;
        if (t == RoadType::Highway && params.placeLotsOnHighways) return true;
        return false;
    }

    LotCollection LotSubdivision::GenerateLots(const RoadNetwork& net, const LotParams& params)
    {
        LotCollection out;
        out.lots.reserve(net.Segments().size() * 4);

        std::mt19937 rng(params.seed);
        std::uniform_real_distribution<float> frontageDist(params.lotMinFrontage, params.lotMaxFrontage);

        LotId nextLotId = 1;

        // Tracks already-placed lots so we can prevent overlaps.
        LotSpatialIndex spatial(params.lotCellSize);

        const auto& nodes = net.Nodes();
        const auto& segs = net.Segments();

        // Precompute node degree (how many segments touch each node)
        std::vector<int> degree(nodes.size(), 0);
        for (const Segment& s : segs)
        {
            if (s.a > 0 && s.a <= (NodeId)nodes.size()) degree[s.a - 1]++;
            if (s.b > 0 && s.b <= (NodeId)nodes.size()) degree[s.b - 1]++;
        }


        for (const Segment& seg : segs)
        {
            if (!IsLotRoadType(seg.type, params))
                continue;

            const glm::vec2 aPos = nodes.at(seg.a - 1).pos;
            const glm::vec2 bPos = nodes.at(seg.b - 1).pos;

            glm::vec2 ab = bPos - aPos;
            float segLen = Length(ab);
            if (segLen <= 1e-3f)
                continue;

            glm::vec2 dir = NormalizeSafe(ab);

            float halfWidth = RoadHalfWidth(seg.type, params);
            float baseOffset = halfWidth + params.lotSetBackFromRoad;
            float depth = params.lotDepth;

            if (seg.type == RoadType::Highway)
            {
                baseOffset = halfWidth + params.highwayLotSetback;
                depth = params.highwayLotDepth;
            }


            // create lots on both sides of the road
            for (int sideSign : { +1, -1 })
            {
                glm::vec2 n = LeftPerp(dir) * (float)sideSign;

                float t = 0.0f;
                while (t < segLen - 1e-3f)
                {
                    float frontage = frontageDist(rng);

                    

                    // clamp to end
                    if (t + frontage > segLen)
                        frontage = segLen - t;

                    // if last remainder is tiny just stop
                    if (frontage < params.lotMinFrontage * 0.5f)
                        break;

                    glm::vec2 p0 = aPos + dir * t;
                    glm::vec2 p1 = aPos + dir * (t + frontage);

                    glm::vec2 mid = 0.5f * (p0 + p1);
                    float jr = params.junctionClearRadius;
                    if (seg.type == RoadType::Highway) jr *= 0.4f;   
                    if (seg.type == RoadType::Street)  jr *= 1.0f;   
                    float jr2 = jr * jr;


                    bool tooCloseToJunction = false;
                    for (size_t ni = 0; ni < nodes.size(); ++ni)
                    {
                        // Only treat actual junctions as "no-lot zones"
                        if (degree[ni] < 3) continue;

                        glm::vec2 d = nodes[ni].pos - mid;
                        if (d.x * d.x + d.y * d.y < jr2)
                        {
                            tooCloseToJunction = true;
                            break;
                        }
                    }
                    

                    if (tooCloseToJunction)
                    {
                        t += frontage;
                        continue;
                    }

                    auto BuildPoly = [&](float depthScale)
                    {
                        glm::vec2 near0 = p0 + n * baseOffset;
                        glm::vec2 near1 = p1 + n * baseOffset;
                        glm::vec2 far1  = p1 + n * (baseOffset + depth * depthScale);
                        glm::vec2 far0  = p0 + n * (baseOffset + depth * depthScale);

                        std::vector<glm::vec2> poly = { near0, near1, far1, far0 };
                        EnsureCCW(poly);
                        return poly;
                    };

                    std::vector<glm::vec2> bestPoly;
                    float bestScale = 0.0f;


                    auto Fits = [&](const std::vector<glm::vec2>& poly)
                    {
                        // no lot-lot overlap
                        OBB2 o = OBBFromLotPoly(poly, params.lotPadding);
                        if (spatial.overlapsAny(o))
                            return false;

                        // no road overlap 
                        OBB2 lotOBB = OBBFromLotPoly(poly, params.lotPadding);

                        for (const Segment& other : segs)
                        {
                            if (other.id == seg.id) continue;

                            const glm::vec2 A = nodes.at(other.a - 1).pos;
                            const glm::vec2 B = nodes.at(other.b - 1).pos;

                            glm::vec2 segMin(std::min(A.x, B.x), std::min(A.y, B.y));
                            glm::vec2 segMax(std::max(A.x, B.x), std::max(A.y, B.y));

                            float inflate = 0.05f;
                            segMin -= glm::vec2(inflate);
                            segMax += glm::vec2(inflate);

                            bool aabbOverlap =
                                !(lotOBB.aabbMax.x < segMin.x || lotOBB.aabbMin.x > segMax.x ||
                                lotOBB.aabbMax.y < segMin.y || lotOBB.aabbMin.y > segMax.y);

                            if (aabbOverlap)
                            {
                                if (LotIntersectsSegment(poly, A, B))
                                    return false;
                            }
                        }

                        return true;
                    };

                    // Compute best polygon for a given frontage
                    auto ComputeBestPoly = [&](float tryFrontage, std::vector<glm::vec2>& outPoly) -> bool
                    {
                        
                        glm::vec2 p1_try = aPos + dir * (t + tryFrontage);

                        auto BuildPoly = [&](float depthScale)
                        {
                            glm::vec2 near0 = p0 + n * baseOffset;
                            glm::vec2 near1 = p1_try + n * baseOffset;
                            glm::vec2 far1  = p1_try + n * (baseOffset + depth * depthScale);
                            glm::vec2 far0  = p0 + n * (baseOffset + depth * depthScale);

                            std::vector<glm::vec2> poly = { near0, near1, far1, far0 };
                            EnsureCCW(poly);
                            return poly;
                        };

                        std::vector<glm::vec2> bestPoly;

                        // try full depth
                        {
                            auto polyFull = BuildPoly(1.0f);
                            if (Fits(polyFull))
                                bestPoly = std::move(polyFull);
                        }

                        // if full depth fails, try shrink depth
                        if (bestPoly.empty())
                        {
                            float lo = params.minDepthScale;
                            float hi = 1.0f;

                            auto candidate = BuildPoly(lo);
                            if (Fits(candidate))
                            {
                                bestPoly = candidate;

                                for (int i = 0; i < params.depthBinarySteps; ++i)
                                {
                                    float mid = 0.5f * (lo + hi);
                                    auto midPoly = BuildPoly(mid);
                                    if (Fits(midPoly))
                                    {
                                        bestPoly = std::move(midPoly);
                                        lo = mid;
                                    }
                                    else
                                    {
                                        hi = mid;
                                    }
                                }
                            }
                        }

                        if (bestPoly.empty())
                            return false;

                        outPoly = std::move(bestPoly);
                        return true;
                    };

                    // Commit a poly as a lot (ONE place)
                    auto CommitLot = [&](std::vector<glm::vec2>&& poly)
                    {
                        float frontage = Length(poly[1] - poly[0]); 
                        float depth    = Length(poly[3] - poly[0]);

                        // Reject extremely skinny lots
                        if (frontage < depth * 0.25f)
                            return;


                        Lot lot;
                        lot.lotId = nextLotId++;
                        lot.roadSegId = seg.id;
                        lot.roadType = seg.type;
                        lot.boundary = std::move(poly);
                        lot.centroid = 0.25f * (lot.boundary[0] + lot.boundary[1] + lot.boundary[2] + lot.boundary[3]);
                        lot.area = std::fabs(SignedArea2D(lot.boundary));


                        //urban score
                        glm::vec2 cityCenter = nodes.empty() ? glm::vec2(0.0f) : nodes[0].pos;
                        float dist01 = 0.0f;
                        if (params.cityRadius > 1e-3f)
                            dist01 = Clamp01(Length(lot.centroid - cityCenter) / params.cityRadius);

                        //near centre => more urban
                        float scoreDist = 1.0f - dist01;
                        //road type bias
                        float scoreRoad = (lot.roadType == RoadType::Highway) ? 0.15f : 0.0f;
                        
                        //area bias: smaller lots read “more urban”
                        float area01 = Clamp01((lot.area - 4.0f) / (30.0f - 4.0f));
                        float scoreArea = 1.0f - area01;

                        //weighted blend
                        float localScore = 
                            0.65f * scoreDist +
                            0.15f * scoreRoad +
                            0.20f * scoreArea;
                        
                        //global slider nudges score up/down
                        float globalShift = (params.globalUrbanization - 0.5f) * 2.0f * params.globalBiasStrength;
                        lot.urbanScore = Clamp01(localScore + globalShift);
                        if (lot.urbanScore >= params.urbanThreshold) lot.zone = LotZone::Urban;
                        else if (lot.urbanScore >= params.suburbanThreshold) lot.zone = LotZone::Suburban;
                        else lot.zone = LotZone::Rural;


                            // garden (only suburban, and only if lot is big enough)
                        lot.hasGarden = false;
                        lot.garden.clear();

                        if (lot.zone == LotZone::Suburban && lot.area >= params.minLotAreaForGarden)
                        {
                            // your quad is near0, near1, far1, far0
                            glm::vec2 near0 = lot.boundary[0];
                            glm::vec2 near1 = lot.boundary[1];
                            glm::vec2 far1  = lot.boundary[2];
                            glm::vec2 far0  = lot.boundary[3];

                            glm::vec2 depthDir = NormalizeSafe(far0 - near0);
                            float fullDepth = Length(far0 - near0);



                            float gardenDepth = fullDepth * params.gardenBackRatio;
                            if (gardenDepth >= params.minGardenDepth && (fullDepth - gardenDepth) >= 0.5f)
                            {
                                // split line position (start of garden, measured from near edge)
                                float splitFromNear = fullDepth - gardenDepth;

                                glm::vec2 mid0 = near0 + depthDir * splitFromNear;
                                glm::vec2 mid1 = near1 + depthDir * splitFromNear;

                                lot.hasGarden = true;
                                lot.garden = { mid0, mid1, far1, far0 };
                                EnsureCCW(lot.garden);
                            }
                        }

                        spatial.insert(OBBFromLotPoly(lot.boundary, params.lotPadding));
                        out.lots.push_back(std::move(lot));
                    };
                
                    std::vector<glm::vec2> chosenPoly;
                    bool placed = false;

                    // try full frontage first
                    if (ComputeBestPoly(frontage, chosenPoly))
                    {
                        CommitLot(std::move(chosenPoly));
                        placed = true;
                    }
                    else
                    {
                        // binary search for the widest frontage that fits
                        float minF = params.lotMinFrontage;
                        float maxF = frontage;
                        float fillCap = std::max(params.lotMinFrontage, maxF * params.fillFrontageFactor);
                        maxF = fillCap;


                        std::vector<glm::vec2> bestPoly;
                        float bestF = 0.0f;

                        // If minimum doesn't fit, give up (advance t by original frontage)
                        std::vector<glm::vec2> polyMin;
                        if (ComputeBestPoly(minF, polyMin))
                        {
                            bestF = minF;
                            bestPoly = std::move(polyMin);

                            float lo = minF;
                            float hi = maxF;

                            const int steps = 8;
                            for (int i = 0; i < steps; ++i)
                            {
                                float midF = 0.5f * (lo + hi);
                                std::vector<glm::vec2> polyMid;

                                if (ComputeBestPoly(midF, polyMid))
                                {
                                    bestF = midF;
                                    bestPoly = std::move(polyMid);
                                    lo = midF;   // try wider
                                }
                                else
                                {
                                    hi = midF;   // try narrower
                                }
                            }

                            frontage = bestF; // IMPORTANT: advance correctly
                            CommitLot(std::move(bestPoly));
                            placed = true;
                        }
                    }

                    // advance along road regardless 
                    if (placed && params.fillExtraAttempts > 0)
                    {

                        // move t to the end of that lot, then attempt a smaller filler lot right away.
                        float savedT = t;
                        float savedFrontage = frontage;

                        t = savedT + savedFrontage;

                        // try to place 1 small filler lot without consuming a new random sample
                        for (int attempt = 0; attempt < params.fillExtraAttempts; ++attempt)
                        {
                            // remaining length on this segment side
                            float remaining = segLen - t;
                            if (remaining < params.lotMinFrontage * 0.5f)
                                break;

                            float fillTarget = std::min(remaining, savedFrontage * params.fillFrontageFactor);
                            fillTarget = std::max(fillTarget, params.lotMinFrontage);

                            std::vector<glm::vec2> fillPoly;

                            // use the same frontage-maximising logic but capped at fillTarget
                            if (ComputeBestPoly(fillTarget, fillPoly))
                            {
                              
                                float lo = params.lotMinFrontage;
                                float hi = fillTarget;
                                float bestF = fillTarget;
                                std::vector<glm::vec2> bestPoly = std::move(fillPoly);

                                const int steps = 6;
                                for (int i = 0; i < steps; ++i)
                                {
                                    float midF = 0.5f * (lo + hi);
                                    std::vector<glm::vec2> midPoly;
                                    if (ComputeBestPoly(midF, midPoly))
                                    {
                                        bestF = midF;
                                        bestPoly = std::move(midPoly);
                                        lo = midF;
                                    }
                                    else
                                    {
                                        hi = midF;
                                    }
                                }

                                CommitLot(std::move(bestPoly));
                                t += bestF; // consume the filler lot
                            }
                            else
                            {
                                // can't fit a filler at this spot
                                break;
                            }
                        }

                        t += frontage;

                        // continue normal progression 
                    }
                    else
                    {
                        t += params.failAdvance;
                    }


                }
            }
        }

        return out;
    }
}
