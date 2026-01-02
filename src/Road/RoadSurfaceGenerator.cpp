
#include "Road/RoadSurfaceGenerator.h"
#include "Road/RoadNetwork.h"
#include "Road/RoadParams.h"
#include "Road/RoadTypes.h"
#include "Road/SideWalkGenerator.h"  

#include <unordered_map>
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace road
{


    static inline glm::vec2 Perp(const glm::vec2& d) { return glm::vec2(-d.y, d.x); }

    static inline glm::vec2 NormalizeSafe(const glm::vec2& v)
    {
        const float l2 = glm::dot(v, v);
        if (l2 < 1e-8f) return glm::vec2(0.0f);
        return v * (1.0f / std::sqrt(l2));
    }

    static inline void AddTri(std::vector<glm::vec3>& v,
                              const glm::vec3& a,
                              const glm::vec3& b,
                              const glm::vec3& c)
    {
        v.push_back(a); v.push_back(b); v.push_back(c);
    }

    static inline void AddQuadAsTwoTris(std::vector<glm::vec3>& v,
                                        const glm::vec3& a,
                                        const glm::vec3& b,
                                        const glm::vec3& c,
                                        const glm::vec3& d)
    {
        AddTri(v, a, b, c);
        AddTri(v, a, c, d);
    }

    // extrude a 2D quad (p0,p1,p2,p3 in XZ plane) upward into triangles.
    static void AddExtrudedQuad(std::vector<glm::vec3>& out,
                                const glm::vec2& p0,
                                const glm::vec2& p1,
                                const glm::vec2& p2,
                                const glm::vec2& p3,
                                float baseY,
                                float height,
                                bool addWalls)
    {
        glm::vec3 t0(p0.x, baseY + height, p0.y);
        glm::vec3 t1(p1.x, baseY + height, p1.y);
        glm::vec3 t2(p2.x, baseY + height, p2.y);
        glm::vec3 t3(p3.x, baseY + height, p3.y);
        AddQuadAsTwoTris(out, t0, t1, t2, t3);

        if (!addWalls) return;

        glm::vec3 b0(p0.x, baseY, p0.y);
        glm::vec3 b1(p1.x, baseY, p1.y);
        glm::vec3 b2(p2.x, baseY, p2.y);
        glm::vec3 b3(p3.x, baseY, p3.y);

        AddQuadAsTwoTris(out, b0, b1, t1, t0);
        AddQuadAsTwoTris(out, b1, b2, t2, t1);
        AddQuadAsTwoTris(out, b2, b3, t3, t2);
        AddQuadAsTwoTris(out, b3, b0, t0, t3);
    }

    // builds a rectangular prism along segment A->B, with optional trimming at each end.
    static void AddExtrudedStrip(std::vector<glm::vec3>& out,
                                 const glm::vec2& A,
                                 const glm::vec2& B,
                                 float centerOffset,
                                 float halfWidth,
                                 float baseY,
                                 float height,
                                 float trimA,
                                 float trimB,
                                 bool addWalls)
    {
        glm::vec2 AB = B - A;
        glm::vec2 d = NormalizeSafe(AB);
        if (glm::dot(d, d) < 1e-8f) return;

        const float segLen = std::sqrt(glm::dot(AB, AB));
        if (segLen <= (trimA + trimB + 1e-3f)) return;

        glm::vec2 n = NormalizeSafe(Perp(d));

        glm::vec2 A2 = A + n * centerOffset + d * trimA;
        glm::vec2 B2 = B + n * centerOffset - d * trimB;

        glm::vec2 L0 = A2 + n * halfWidth;
        glm::vec2 R0 = A2 - n * halfWidth;
        glm::vec2 L1 = B2 + n * halfWidth;
        glm::vec2 R1 = B2 - n * halfWidth;

        AddExtrudedQuad(out, L0, R0, R1, L1, baseY, height, addWalls);
    }

void BuildRoadSurfaceTriVerts(const RoadNetwork& net,
                              const RoadParams& params,
                              std::vector<glm::vec3>& outHighwayTris,
                              std::vector<glm::vec3>& outStreetTris,
                              float roadBaseY,
                              float roadHeight)
{
    outHighwayTris.clear();
    outStreetTris.clear();

    const auto& nodes = net.Nodes();
    const auto& segs  = net.Segments();
    if (nodes.empty() || segs.empty()) return;

    // degree 
    std::vector<int> deg(nodes.size(), 0);
    for (const auto& s : segs)
    {
        if (s.a > 0 && (size_t)s.a <= nodes.size()) deg[(size_t)s.a - 1]++;
        if (s.b > 0 && (size_t)s.b <= nodes.size()) deg[(size_t)s.b - 1]++;
    }

    outHighwayTris.reserve(segs.size() * 6);
    outStreetTris.reserve(segs.size() * 6);

    for (const auto& s : segs)
    {
        const glm::vec2 A = nodes.at((size_t)s.a - 1).pos;
        const glm::vec2 B = nodes.at((size_t)s.b - 1).pos;

        const float halfW = (s.type == RoadType::Highway)
                          ? params.highwayHalfWidth
                          : params.streetHalfWidth;

        const float segLen = std::sqrt(glm::dot(B - A, B - A));
        if (segLen < 1e-6f) continue;

        // base extension size 
        float extendBase = std::min(halfW, segLen * 0.45f);

        // only extend into junctions
        const bool A_isJunction = (deg[(size_t)s.a - 1] >= 2);
        const bool B_isJunction = (deg[(size_t)s.b - 1] >= 2);

        float extendA = A_isJunction ? extendBase : 0.0f;
        float extendB = B_isJunction ? extendBase : 0.0f;

        // negative trims = extend beyond endpoints
        const float trimA = -extendA;
        const float trimB = -extendB;

        if (s.type == RoadType::Highway)
            AddExtrudedStrip(outHighwayTris, A, B, 0.0f, halfW, roadBaseY, roadHeight, trimA, trimB, true);
        else
            AddExtrudedStrip(outStreetTris, A, B, 0.0f, halfW, roadBaseY, roadHeight, trimA, trimB, true);
    }
}


    void BuildSidewalkSurfaceTriVerts(const RoadNetwork& net,
                                    const RoadParams& params,
                                    std::vector<glm::vec3>& outSidewalkTris,
                                    float sidewalkBaseY,
                                    float sidewalkHeight,
                                    float,
                                    bool)
    {
        outSidewalkTris.clear();

        // build sidewalk line segments (your existing logic)
        std::vector<glm::vec3> sidewalkLines;
        sidewalkLines.reserve(net.Segments().size() * 8);
        BuildSidewalkLineVerts(net, params, sidewalkLines, 0.0f);

        if (sidewalkLines.size() < 2) return;

        const float swHalfW = params.sidewalkWidth * 0.5f;
        outSidewalkTris.reserve((sidewalkLines.size() / 2) * 12);

        
        const float cell = std::max(0.05f, swHalfW * 0.5f);
        const float invCell = 1.0f / cell;

        auto KeyFor = [&](const glm::vec2& p) -> std::int64_t
        {
            // quantize to a grid to cluster near-equal endpoints
            const int xi = (int)std::floor(p.x * invCell + 0.5f);
            const int yi = (int)std::floor(p.y * invCell + 0.5f);

            // pack two 32-bit ints into one 64-bit key
            return (std::int64_t(std::uint32_t(xi)) << 32) | std::uint32_t(yi);
        };

        std::unordered_map<std::int64_t, int> endpointUseCount;
        endpointUseCount.reserve(sidewalkLines.size());

 
        for (size_t i = 0; i + 1 < sidewalkLines.size(); i += 2)
        {
            glm::vec2 A(sidewalkLines[i].x,     sidewalkLines[i].z);
            glm::vec2 B(sidewalkLines[i + 1].x, sidewalkLines[i + 1].z);

            endpointUseCount[KeyFor(A)]++;
            endpointUseCount[KeyFor(B)]++;
        }

 
        for (size_t i = 0; i + 1 < sidewalkLines.size(); i += 2)
        {
            const glm::vec3 a3 = sidewalkLines[i];
            const glm::vec3 b3 = sidewalkLines[i + 1];

            glm::vec2 A(a3.x, a3.z);
            glm::vec2 B(b3.x, b3.z);

            const float segLen = std::sqrt(glm::dot(B - A, B - A));
            if (segLen < 1e-6f) continue;

            // overlap just enough to hide the "bite"
            const float extendBase = std::min(swHalfW, segLen * 0.45f);

            const bool A_isJunction = (endpointUseCount[KeyFor(A)] >= 2);
            const bool B_isJunction = (endpointUseCount[KeyFor(B)] >= 2);

            const float extendA = A_isJunction ? extendBase : 0.0f;
            const float extendB = B_isJunction ? extendBase : 0.0f;

            // negative trims = extend beyond endpoints
            const float trimA = -extendA;
            const float trimB = -extendB;

            AddExtrudedStrip(outSidewalkTris, A, B,
                            0.0f, swHalfW,
                            sidewalkBaseY, sidewalkHeight,
                            trimA, trimB,
                            true);
        }
    }


}