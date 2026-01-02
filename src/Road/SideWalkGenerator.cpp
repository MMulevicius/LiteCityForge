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

    static bool IntersectRays(const Line2& a, const Line2& b, glm::vec2& out)
    {
        float det = a.dir.x * b.dir.y - a.dir.y * b.dir.x;
        if (std::fabs(det) < 1e-6f) return false;

        glm::vec2 r = b.p - a.p;

        float t = (r.x * b.dir.y - r.y * b.dir.x) / det;
        float u = (r.x * a.dir.y - r.y * a.dir.x) / det;

        if (t < 0.0f || u < 0.0f) return false;

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

    static inline bool JoinTooFar(const glm::vec2& P,
                                const glm::vec2& dirOut,
                                RoadType type,
                                const RoadParams& params,
                                const glm::vec2& x)
    {
        // forward distance from node along the road direction
        float t = Dot(x - P, dirOut);

        // cap forward movement to about the curb offset distance (+ tiny slack)
        float curbOff = RoadHalfW(type, params) + params.sidewalkGap + params.sidewalkWidth * 0.5f;
        float maxAdvance = curbOff + 0.02f;

        return t > maxAdvance;
    }


    static inline float Clamp01(float x) { return std::max(-1.0f, std::min(1.0f, x)); }

    static inline float SinHalfAngle(const glm::vec2& a, const glm::vec2& b)
    {
        // a and b assumed normalized
        float c = Clamp01(Dot(a, b));
        // sin(theta/2) = sqrt((1 - cos(theta))/2)
        return std::sqrt(std::max(0.0f, (1.0f - c) * 0.5f));
    }

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


            if (E.size() == 2)
            {
                const Entry& e0 = E[0];
                const Entry& e1 = E[1];

                Line2 e0L = leftLine(e0);
                Line2 e0R = rightLine(e0);
                Line2 e1L = leftLine(e1);
                Line2 e1R = rightLine(e1);

                glm::vec2 xA, xB;
                bool okA = IntersectLines(e0L, e1R, xA); 
                bool okB = IntersectLines(e0R, e1L, xB); 

                float clampDist = std::max(e0.off, e1.off) * miterLimit;

                if (okA) xA = ClampMiter(xA, P, clampDist);
                if (okB) xB = ClampMiter(xB, P, clampDist);

                if (okA && okB)
                {
                    float dA = LenSq(xA - P);
                    float dB = LenSq(xB - P);

                    // outer = farther
                    glm::vec2 outer = (dA > dB) ? xA : xB;
                    glm::vec2 inner = (dA > dB) ? xB : xA;

                    bool outerIsA = (dA > dB);

                    if (outerIsA)
                    {
                        // outer came from (e0L, e1R), inner from (e0R, e1L)
                        joins[ni][e0.segId].L = outer; joins[ni][e0.segId].hasL = true;
                        joins[ni][e1.segId].R = outer; joins[ni][e1.segId].hasR = true;

                        joins[ni][e0.segId].R = inner; joins[ni][e0.segId].hasR = true;
                        joins[ni][e1.segId].L = inner; joins[ni][e1.segId].hasL = true;
                    }
                    else
                    {
                        // outer came from (e0R, e1L), inner from (e0L, e1R)
                        joins[ni][e0.segId].R = outer; joins[ni][e0.segId].hasR = true;
                        joins[ni][e1.segId].L = outer; joins[ni][e1.segId].hasL = true;

                        joins[ni][e0.segId].L = inner; joins[ni][e0.segId].hasL = true;
                        joins[ni][e1.segId].R = inner; joins[ni][e1.segId].hasR = true;
                    }
                }
                else
                {
                    glm::vec2 p0L = e0L.p, p0R = e0R.p;
                    glm::vec2 p1L = e1L.p, p1R = e1R.p;

                    joins[ni][e0.segId].L = p0L; joins[ni][e0.segId].hasL = true;
                    joins[ni][e0.segId].R = p0R; joins[ni][e0.segId].hasR = true;

                    joins[ni][e1.segId].L = p1L; joins[ni][e1.segId].hasL = true;
                    joins[ni][e1.segId].R = p1R; joins[ni][e1.segId].hasR = true;
                }

                continue;
            }


            const int k = (int)E.size();
            for (int i = 0; i < k; ++i)
            {
                const Entry& cur  = E[i];
                const Entry& next = E[(i + 1) % k];

                Line2 a = leftLine(cur);
                Line2 b = rightLine(next);

                glm::vec2 x;
                bool ok = IntersectLines(a, b, x);

                // always clamp miters first 
                if (ok)
                {
                    float clampDist = std::max(cur.off, next.off) * miterLimit;
                    x = ClampMiter(x, P, clampDist);

               
                    float tCur  = Dot(x - P, cur.dirOut);
                    float tNext = Dot(x - P, next.dirOut);

                    // allow forward advance up to road half-width + sidewalk offset
                    float maxCur  = RoadHalfW(cur.type,  params) + 0.75f * cur.off;
                    float maxNext = RoadHalfW(next.type, params) + 0.75f * next.off;

                    // small slack so it doesn't jitter
                    const float slack = 0.02f;

                    // build two adjusted candidates (one respecting each road's max advance)
                    glm::vec2 xCur  = x;
                    glm::vec2 xNext = x;

                    if (tCur > maxCur + slack)
                        xCur -= cur.dirOut * (tCur - (maxCur + slack));

                    if (tNext > maxNext + slack)
                        xNext -= next.dirOut * (tNext - (maxNext + slack));

                    // if either clamp happened, blend them (keeps a single shared join)
                    if (LenSq(xCur - x) > 1e-10f || LenSq(xNext - x) > 1e-10f)
                        x = 0.5f * (xCur + xNext);

                }

                if (ok)
                {
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
                else
                {
                    // no shared join point -> each keeps its own clean offset at the node
                    {
                        JoinLR& J = joins[ni][cur.segId];
                        J.L = a.p;
                        J.hasL = true;
                    }
                    {
                        JoinLR& J = joins[ni][next.segId];
                        J.R = b.p;
                        J.hasR = true;
                    }
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