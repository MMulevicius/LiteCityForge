#include "Road/LotSubdivision.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace road
{
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
        double a = 0.0f;
        for (size_t i = 0; i < poly.size(); i++)
        {
            const glm::vec2& p = poly[i];
            const glm::vec2& q = poly[(i + 1) % poly.size()];
            a += (double)p.x * (double)q.y - (double)q.x * (double)p.y;
        }
        return (float)(0.5 * a);
    }

    static void EnsureCCW(std::vector<glm::vec2>& poly)
    {
        if (SignedArea2D(poly) < 0.0f)
            std::reverse(poly.begin(), poly.end());
    } 

    static float RoadHalfWidth(RoadType type, const LotParams& p)
    {
        return (type == RoadType::Street) ? p.streetHalfWidth : p.highwayHalfWidth;
    }

    bool LotSubdivision::IsLotRoadType(RoadType t, const LotParams& params)
    {
        if (t == RoadType::Street && params.placeLotsOnStreets) return true;
        if (t == RoadType::Street && params.placeLotsOnHighways) return true;
        return false;
    }

    LotCollection LotSubdivision::GenerateLots(const RoadNetwork& net, const LotParams& params)
    {
        LotCollection out;
        // rough guess
        out.lots.reserve(net.Segments().size() * 4);
        
        std::mt19937 rng(params.seed);
        std::uniform_real_distribution<float> frontageDist(params.lotMinFrontage, params.lotMaxFrontage);

        LotId nextLotId = 1;

        const auto& nodes = net.Nodes();
        const auto& segs = net.Segments();

        for(const Segment& seg : segs)
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

            // create lots on both sides of the road
            for (int sideSign : { +1, -1})
            {
                glm::vec2 n = LeftPerp(dir) * (float)sideSign;

                float t = 0.0f;
                while (t < segLen - 1e-3f)
                {
                    float frontage = frontageDist(rng);

                    //clamp to end
                    if (t + frontage > segLen)
                        frontage = segLen - t;

                    // if last remainder is tiny just stop
                    if (frontage < params.lotMinFrontage * 0.5f)
                        break;

                    // points along the segment
                    glm::vec2 p0 = aPos + dir * t;
                    glm::vec2 p1 = aPos + dir * (t + frontage);

                    // build quad lot polygon
                    glm::vec2 near0 = p0 + n * baseOffset;
                    glm::vec2 near1 = p1 + n * baseOffset;
                    glm::vec2 far1 = p1 + n * (baseOffset + depth);
                    glm::vec2 far0 = p0 + n * (baseOffset + depth);

                    std::vector<glm::vec2> poly = { near0, near1, far1, far0};
                    EnsureCCW(poly);

                    Lot lot;
                    lot.lotId = nextLotId++;
                    lot.roadSegId = seg.id;
                    lot.roadType = seg.type;
                    lot.boundary = std::move(poly);


                    out.lots.push_back(std::move(lot));

                    t+= frontage;
                }
            }
        }

        return out;
    }

}