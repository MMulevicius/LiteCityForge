#include "Road/SideWalkGenerator.h"
#include "Road/RoadNetwork.h"
#include "Road/RoadParams.h"
#include "Road/RoadTypes.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace road
{
    static inline float Dot(const glm::vec2& a, const glm::vec2& b) { return a.x * b.x + a.y * b.y; }
    static inline float LenSq(const glm::vec2& v) { return Dot(v, v); }

    static inline glm::vec2 Perp(const glm::vec2& d) { return glm::vec2(-d.y, d.x); }

    static inline glm::vec2 NormalizeSafe(const glm::vec2& v)
    {
        float l2 = LenSq(v);
        if (l2 < 1e-8f) return glm::vec2(0.0f);
        return v * (1.0f / std::sqrt(l2));
    }

    static inline void AddLine(std::vector<glm::vec3>& out, const glm::vec2& a, const glm::vec2& b, float y)
    {
        out.emplace_back(a.x, y, a.y);
        out.emplace_back(b.x, y, b.y);
    }

    struct Line2
    {
        glm::vec2 p;
        glm::vec2 dir;
    };

    static bool IntersectLines(const Line2& a, const Line2& b, glm::vec2& out)
    {
        float det = a.dir.x * b.dir.y - a.dir.y * b.dir.x;
        if (std::fabs(det) < 1e-6f) return false;

        glm::vec2 r = b.p - a.p;
        float t = (r.x * b.dir.y - r.y * b.dir.x) / det;
        out = a.p + a.dir * t;
        return true;
    }

    static glm::vec2 ClampMiter(const glm::vec2& p, const glm::vec2& center, float maxDist)
    {
        glm::vec2 v = p - center;
        float d2 = LenSq(v);
        if (d2 <= maxDist * maxDist) return p;

        float d = std::sqrt(std::max(d2, 1e-8f));
        return center + v * (maxDist / d);
    }

    static inline float RoadHalfW(RoadType t, const RoadParams& p)
    {
        return (t == RoadType::Highway) ? p.highwayHalfWidth : p.streetHalfWidth;
    }

    struct JoinLR
    {
        glm::vec2 L; 
        glm::vec2 R; 
        bool hasL = false;
        bool hasR = false;
    };

    void BuildSidewalkLineVerts(const RoadNetwork& net, const RoadParams& params,
                                std::vector<glm::vec3>& outSidewalks, float y)
    {
        outSidewalks.clear();

        const auto& nodes = net.Nodes();
        const auto& segs  = net.Segments();
        if (nodes.empty() || segs.empty()) return;

        // incident segments per node + degree
        std::vector<std::vector<size_t>> incident(nodes.size());
        std::vector<int> degree(nodes.size(), 0);

        for (size_t si = 0; si < segs.size(); ++si)
        {
            const auto& s = segs[si];
            if (s.a > 0 && (size_t)s.a <= nodes.size())
            {
                incident[(size_t)s.a - 1].push_back(si);
                degree[(size_t)s.a - 1]++;
            }
            if (s.b > 0 && (size_t)s.b <= nodes.size())
            {
                incident[(size_t)s.b - 1].push_back(si);
                degree[(size_t)s.b - 1]++;
            }
        }

        // joins[nodeIdx][segId] -> JoinLR
        std::vector<std::unordered_map<SegId, JoinLR>> joins(nodes.size());

        // miter clamp multiplier (bigger = sharper corners, smaller = safer)
        const float miterLimit = 4.0f;

        // build junction joins
        for (size_t ni = 0; ni < nodes.size(); ++ni)
        {
            auto& inc = incident[ni];
            if (inc.size() < 2) continue;

            const glm::vec2 P = nodes[ni].pos;

            struct Entry
            {
                SegId segId;
                RoadType type;
                glm::vec2 dirOut;
                float ang;
                float off; 
            };

            std::vector<Entry> E;
            E.reserve(inc.size());

            for (size_t si : inc)
            {
                const auto& s = segs[si];

                glm::vec2 A = nodes[(size_t)s.a - 1].pos;
                glm::vec2 B = nodes[(size_t)s.b - 1].pos;

                glm::vec2 d;
                if ((size_t)s.a - 1 == ni) d = NormalizeSafe(B - A);
                else                       d = NormalizeSafe(A - B);

                if (LenSq(d) < 1e-8f) continue;

                float off = RoadHalfW(s.type, params) + params.sidewalkGap + params.sidewalkWidth * 0.5f;
                float ang = std::atan2(d.y, d.x);
                E.push_back({ s.id, s.type, d, ang, off });
            }

            if (E.size() < 2) continue;

            std::sort(E.begin(), E.end(), [](const Entry& a, const Entry& b){ return a.ang < b.ang; });


            const int k = (int)E.size();
            for (int i = 0; i < k; ++i)
            {
                const Entry& cur  = E[i];
                const Entry& next = E[(i + 1) % k];

                auto leftLine = [&](const Entry& e) -> Line2
                {
                    glm::vec2 n = NormalizeSafe(Perp(e.dirOut));
                    return Line2{ P + n * e.off, e.dirOut };
                };

                auto rightLine = [&](const Entry& e) -> Line2
                {
                    glm::vec2 n = NormalizeSafe(Perp(e.dirOut));
                    return Line2{ P - n * e.off, e.dirOut };
                };

                Line2 a = leftLine(cur);
                Line2 b = rightLine(next);

                glm::vec2 x;
                if (!IntersectLines(a, b, x))
                {
                    // fallback: just use cur's left offset point
                    x = a.p;
                }

                float clampDist = std::max(cur.off, next.off) * miterLimit;
                x = ClampMiter(x, P, clampDist);

                // cur gets left join at this node
                {
                    JoinLR& J = joins[ni][cur.segId];
                    J.L = x;
                    J.hasL = true;
                }
                // next gets right join at this node
                {
                    JoinLR& J = joins[ni][next.segId];
                    J.R = x;
                    J.hasR = true;
                }
            }
        }

        // emit sidewalk centerlines per segment
        outSidewalks.reserve(segs.size() * 8);

        for (const auto& s : segs)
        {
            const size_t ia = (size_t)s.a - 1;
            const size_t ib = (size_t)s.b - 1;

            const glm::vec2 A = nodes[ia].pos;
            const glm::vec2 B = nodes[ib].pos;

            glm::vec2 dAB = NormalizeSafe(B - A);
            if (LenSq(dAB) < 1e-8f) continue;

            glm::vec2 nAB = NormalizeSafe(Perp(dAB));

            float off = RoadHalfW(s.type, params) + params.sidewalkGap + params.sidewalkWidth * 0.5f;

            // fallback endpoints if no join exists
            glm::vec2 leftA  = A + nAB * off;
            glm::vec2 rightA = A - nAB * off;
            glm::vec2 leftB  = B + nAB * off;
            glm::vec2 rightB = B - nAB * off;

            // at node A: dirOut == +dAB, so L/R mapping is direct
            {
                auto it = joins[ia].find(s.id);
                if (it != joins[ia].end())
                {
                    if (it->second.hasL) leftA  = it->second.L;
                    if (it->second.hasR) rightA = it->second.R;
                }
            }

            // at node B: node-local dirOut was -dAB, so left/right swap relative to A->B
            {
                auto it = joins[ib].find(s.id);
                if (it != joins[ib].end())
                {
                    // swapped:
                    if (it->second.hasL) rightB = it->second.L;
                    if (it->second.hasR) leftB  = it->second.R;
                }
            }

            // two sidewalk centerlines
            AddLine(outSidewalks, leftA,  leftB,  y);
            AddLine(outSidewalks, rightA, rightB, y);

            // dead-end caps (degree 1): connect left<->right at endpoint
            if (degree[ia] == 1) AddLine(outSidewalks, leftA, rightA, y);
            if (degree[ib] == 1) AddLine(outSidewalks, leftB, rightB, y);
        }
    }

} 
