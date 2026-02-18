#include "Rendering/BuildingRenderer.h"
#include "Lots/LotSubdivision.h"   
#include <glm/gtc/type_ptr.hpp>
#include "Rendering/Shader.h"
#include <array>       
#include <algorithm>  
#include <cstddef>

namespace road
{
    bool BuildingRenderer::Initialize()
    {
        glGenVertexArrays(1, &mVAO);
        glGenBuffers(1, &mVBO);

        glBindVertexArray(mVAO);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);

        // position only
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        return mVAO != 0 && mVBO != 0;
    }

    void BuildingRenderer::Shutdown()
    {
        if (mVBO) glDeleteBuffers(1, &mVBO);
        if (mVAO) glDeleteVertexArrays(1, &mVAO);
        mVBO = 0;
        mVAO = 0;
        mVertexCount = 0;
    }

    void BuildingRenderer::Upload(const std::vector<glm::vec3>& triVerts)
    {
        mVertexCount = triVerts.size();

        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     (GLsizeiptr)(triVerts.size() * sizeof(glm::vec3)),
                     triVerts.data(),
                     GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void BuildingRenderer::Draw(const Shader& shader, const glm::mat4& vp) const
    {
        if (mVertexCount == 0) return;

        shader.use();
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uVP"), 1, GL_FALSE, glm::value_ptr(vp));

        glm::mat4 model(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uModel"), 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(mVAO);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)mVertexCount);
        glBindVertexArray(0);
    }


    // Texture building renderer
    bool BuildingTexturedRenderer::Initialize()
    {
        glGenVertexArrays(1, &mVAO);
        glGenBuffers(1, &mVBO);

        glBindVertexArray(mVAO);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);

        // layout:
        // location 0 -> vec3 position
        // location 1 -> vec2 uv
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BuildingVertexPT), (void*)offsetof(BuildingVertexPT, pos));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(BuildingVertexPT), (void*)offsetof(BuildingVertexPT, uv));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        return mVAO != 0 && mVBO != 0;
    }

    void BuildingTexturedRenderer::Shutdown()
    {
        if (mVBO) glDeleteBuffers(1, &mVBO);
        if (mVAO) glDeleteVertexArrays(1, &mVAO);
        mVBO = 0;
        mVAO = 0;
        mVertexCount = 0;
    }

    void BuildingTexturedRenderer::Upload(const std::vector<BuildingVertexPT>& triVerts)
    {
        mVertexCount = triVerts.size();

        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     (GLsizeiptr)(triVerts.size() * sizeof(BuildingVertexPT)),
                     triVerts.data(),
                     GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void BuildingTexturedRenderer::Draw(const Shader& shader,
                                        const glm::mat4& vp,
                                        GLuint textureId,
                                        bool useTexture,
                                        const glm::vec3& fallbackColor) const
    {
        if (mVertexCount == 0) return;

        shader.use();
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uVP"), 1, GL_FALSE, glm::value_ptr(vp));

        glm::mat4 model(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uModel"), 1, GL_FALSE, glm::value_ptr(model));

        glUniform1i(glGetUniformLocation(shader.ID, "uUseTexture"), useTexture ? 1 : 0);
        glUniform3f(glGetUniformLocation(shader.ID, "uColor"), fallbackColor.r, fallbackColor.g, fallbackColor.b);

        // texture (optional)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, useTexture ? textureId : 0);
        glUniform1i(glGetUniformLocation(shader.ID, "uTexture"), 0);

        glBindVertexArray(mVAO);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)mVertexCount);
        glBindVertexArray(0);
    }
    
    // mesh building helpers

    static inline glm::vec3 XZToVec3(const glm::vec2& p, float y)
    {
        return glm::vec3(p.x, y, p.y);
    }

    static inline void AddTri(std::vector<glm::vec3>& v,
                              const glm::vec3& a,
                              const glm::vec3& b,
                              const glm::vec3& c)
    {
        v.push_back(a);
        v.push_back(b);
        v.push_back(c);
    }

    static inline void AddQuad(std::vector<glm::vec3>& v,
                                const glm::vec3& a,
                                const glm::vec3& b,
                                const glm::vec3& c,
                                const glm::vec3& d)
    {
        v.push_back(a);
        v.push_back(b);
        v.push_back(c);
        v.push_back(d);
    }

    static inline void AddQuadAsTwoTris(std::vector<glm::vec3>& v,
                                        const glm::vec3& a,
                                        const glm::vec3& b,
                                        const glm::vec3& c,
                                        const glm::vec3& d)
    {
        // a-b-c and a-c-d
        AddTri(v, a, b, c);
        AddTri(v, a, c, d);
    }

    void BuildBuildingTriVerts(const LotCollection& lots,
                               std::vector<glm::vec3>& outTriVerts,
                               float baseY,
                               float floorHeight)
    {
        outTriVerts.clear();
        outTriVerts.reserve(lots.lots.size() * 6 * 3); // rough reserve

        for (const auto& lot : lots.lots)
        {
            if (!lot.hasFootPrint) continue;
            if (lot.footprint.size() != 4) continue;

            const float h = std::max(1, lot.floors) * floorHeight;

            // footprint quad points (2D)
            const glm::vec2& p0 = lot.footprint[0];
            const glm::vec2& p1 = lot.footprint[1];
            const glm::vec2& p2 = lot.footprint[2];
            const glm::vec2& p3 = lot.footprint[3];

            // bottom ring
            glm::vec3 b0 = XZToVec3(p0, baseY);
            glm::vec3 b1 = XZToVec3(p1, baseY);
            glm::vec3 b2 = XZToVec3(p2, baseY);
            glm::vec3 b3 = XZToVec3(p3, baseY);

            // top ring
            glm::vec3 t0 = XZToVec3(p0, baseY + h);
            glm::vec3 t1 = XZToVec3(p1, baseY + h);
            glm::vec3 t2 = XZToVec3(p2, baseY + h);
            glm::vec3 t3 = XZToVec3(p3, baseY + h);

            // walls (each edge becomes a quad)
            AddQuadAsTwoTris(outTriVerts, b0, b1, t1, t0);
            AddQuadAsTwoTris(outTriVerts, b1, b2, t2, t1);
            AddQuadAsTwoTris(outTriVerts, b2, b3, t3, t2);
            AddQuadAsTwoTris(outTriVerts, b3, b0, t0, t3);

            // roof (top quad)
            AddQuadAsTwoTris(outTriVerts, t0, t1, t2, t3);
        }
    }

    void BuildBuildingQuadVerts(const LotCollection& lots,
                            std::vector<glm::vec3>& outQuadVerts,
                            float baseY,
                            float floorHeight)
    {
        outQuadVerts.clear();

        for (const auto& lot : lots.lots)
        {
            if (!lot.hasFootPrint) continue;
            if (lot.footprint.size() != 4) continue;

            const float h = std::max(1, lot.floors) * floorHeight;

            const glm::vec2& p0 = lot.footprint[0];
            const glm::vec2& p1 = lot.footprint[1];
            const glm::vec2& p2 = lot.footprint[2];
            const glm::vec2& p3 = lot.footprint[3];

            glm::vec3 b0 = XZToVec3(p0, baseY);
            glm::vec3 b1 = XZToVec3(p1, baseY);
            glm::vec3 b2 = XZToVec3(p2, baseY);
            glm::vec3 b3 = XZToVec3(p3, baseY);

            glm::vec3 t0 = XZToVec3(p0, baseY + h);
            glm::vec3 t1 = XZToVec3(p1, baseY + h);
            glm::vec3 t2 = XZToVec3(p2, baseY + h);
            glm::vec3 t3 = XZToVec3(p3, baseY + h);

            // walls (quads)
            AddQuad(outQuadVerts, b0, b1, t1, t0);
            AddQuad(outQuadVerts, b1, b2, t2, t1);
            AddQuad(outQuadVerts, b2, b3, t3, t2);
            AddQuad(outQuadVerts, b3, b0, t0, t3);

            // roof (quad)
            AddQuad(outQuadVerts, t0, t1, t2, t3);
        }
    }


    //textured mesh building

    static inline void AddTriPT(std::vector<BuildingVertexPT>& v,
                               const glm::vec3& aPos, const glm::vec2& aUV,
                               const glm::vec3& bPos, const glm::vec2& bUV,
                               const glm::vec3& cPos, const glm::vec2& cUV)
    {
        v.push_back({aPos, aUV});
        v.push_back({bPos, bUV});
        v.push_back({cPos, cUV});
    }

    static inline void AddQuadAsTwoTrisPT(std::vector<BuildingVertexPT>& v,
                                         const glm::vec3& aPos, const glm::vec2& aUV,
                                         const glm::vec3& bPos, const glm::vec2& bUV,
                                         const glm::vec3& cPos, const glm::vec2& cUV,
                                         const glm::vec3& dPos, const glm::vec2& dUV)
    {
        AddTriPT(v, aPos, aUV, bPos, bUV, cPos, cUV);
        AddTriPT(v, aPos, aUV, cPos, cUV, dPos, dUV);
    }

    void BuildBuildingTriVertsTexturedByZone(const LotCollection& lots,
                                             std::vector<BuildingVertexPT>& outUrban,
                                             std::vector<BuildingVertexPT>& outSuburban,
                                             std::vector<BuildingVertexPT>& outRural,
                                             float baseY,
                                             float floorHeight,
                                             float uvMetersPerTile)
    {
        outUrban.clear();
        outSuburban.clear();
        outRural.clear();

        if (uvMetersPerTile <= 0.001f) uvMetersPerTile = 2.0f;
        const float uvScale = 1.0f / uvMetersPerTile;

        auto ZoneVec = [&](road::LotZone z) -> std::vector<BuildingVertexPT>&
        {
            switch (z)
            {
                case road::LotZone::Urban:    return outUrban;
                case road::LotZone::Suburban: return outSuburban;
                case road::LotZone::Rural:    return outRural;
                default:                      return outSuburban;
            }
        };

        for (const auto& lot : lots.lots)
        {
            if (!lot.hasFootPrint) continue;
            if (lot.footprint.size() != 4) continue;

            auto& out = ZoneVec(lot.zone);
            const float h = std::max(1, lot.floors) * floorHeight;

            const glm::vec2& p0 = lot.footprint[0];
            const glm::vec2& p1 = lot.footprint[1];
            const glm::vec2& p2 = lot.footprint[2];
            const glm::vec2& p3 = lot.footprint[3];

            glm::vec3 b0 = XZToVec3(p0, baseY);
            glm::vec3 b1 = XZToVec3(p1, baseY);
            glm::vec3 b2 = XZToVec3(p2, baseY);
            glm::vec3 b3 = XZToVec3(p3, baseY);

            glm::vec3 t0 = XZToVec3(p0, baseY + h);
            glm::vec3 t1 = XZToVec3(p1, baseY + h);
            glm::vec3 t2 = XZToVec3(p2, baseY + h);
            glm::vec3 t3 = XZToVec3(p3, baseY + h);

            auto wallUVs = [&](const glm::vec2& a2, const glm::vec2& b2)
            {
                const float edgeLen = glm::length(b2 - a2);
                const float u1 = edgeLen * uvScale;
                const float v1 = h * uvScale;
                return std::array<glm::vec2,4>{
                    glm::vec2(0.0f, 0.0f),
                    glm::vec2(u1, 0.0f),
                    glm::vec2(u1, v1),
                    glm::vec2(0.0f, v1)
                };
            };

            // walls
            { auto uv = wallUVs(p0, p1); AddQuadAsTwoTrisPT(out, b0, uv[0], b1, uv[1], t1, uv[2], t0, uv[3]); }
            { auto uv = wallUVs(p1, p2); AddQuadAsTwoTrisPT(out, b1, uv[0], b2, uv[1], t2, uv[2], t1, uv[3]); }
            { auto uv = wallUVs(p2, p3); AddQuadAsTwoTrisPT(out, b2, uv[0], b3, uv[1], t3, uv[2], t2, uv[3]); }
            { auto uv = wallUVs(p3, p0); AddQuadAsTwoTrisPT(out, b3, uv[0], b0, uv[1], t0, uv[2], t3, uv[3]); }

            // roof planar UV [0..1]
            float minX = std::min(std::min(p0.x, p1.x), std::min(p2.x, p3.x));
            float maxX = std::max(std::max(p0.x, p1.x), std::max(p2.x, p3.x));
            float minZ = std::min(std::min(p0.y, p1.y), std::min(p2.y, p3.y));
            float maxZ = std::max(std::max(p0.y, p1.y), std::max(p2.y, p3.y));
            const float dx = std::max(0.001f, maxX - minX);
            const float dz = std::max(0.001f, maxZ - minZ);

            auto roofUV = [&](const glm::vec2& p) -> glm::vec2
            {
                return glm::vec2((p.x - minX) / dx, (p.y - minZ) / dz);
            };

            const glm::vec2 uv0 = roofUV(p0);
            const glm::vec2 uv1 = roofUV(p1);
            const glm::vec2 uv2 = roofUV(p2);
            const glm::vec2 uv3 = roofUV(p3);

            AddQuadAsTwoTrisPT(out, t0, uv0, t1, uv1, t2, uv2, t3, uv3);
        }
    }

    // roof + window
    static inline glm::vec2 ToVec2(const glm::vec3& v) { return {v.x, v.z}; }

    static inline glm::vec2 SafeNorm2(const glm::vec2& v)
    {
        float len = glm::length(v);
        if (len < 1e-6f) return glm::vec2(1, 0);
        return v / len;
    }

    static inline float SignedArea2(const std::vector<glm::vec2>& poly)
    {
        float a = 0.0f;
        for (size_t i = 0; i < poly.size(); ++i)
        {
            const auto& p = poly[i];
            const auto& q = poly[(i + 1) % poly.size()];
            a += (p.x * q.y - q.x * p.y);
        }
        return 0.5f * a;
    }

    static inline glm::vec2 InsetTowardCentroid(const glm::vec2& p, const glm::vec2& c, float inset)
    {
        glm::vec2 dir = c - p;
        float len = glm::length(dir);
        if (len < 1e-6f) return p;
        return p + (dir / len) * inset;
    }


    void BuildRoofDetailTriVertsTexturedByZone(const LotCollection& lots,
                                           std::vector<BuildingVertexPT>& outUrban,
                                           std::vector<BuildingVertexPT>& outSuburban,
                                           std::vector<BuildingVertexPT>& outRural,
                                           float baseY,
                                           float floorHeight,
                                           float uvMetersPerTile)
    {
        outUrban.clear();
        outSuburban.clear();
        outRural.clear();

        if (uvMetersPerTile <= 0.001f) uvMetersPerTile = 2.0f;
        const float uvScale = 1.0f / uvMetersPerTile;

        auto ZoneVec = [&](road::LotZone z) -> std::vector<BuildingVertexPT>&
        {
            switch (z)
            {
                case road::LotZone::Urban:    return outUrban;
                case road::LotZone::Suburban: return outSuburban;
                case road::LotZone::Rural:    return outRural;
                default:                      return outSuburban;
            }
        };

        auto UV_WorldXZ = [&](const glm::vec3& p) -> glm::vec2
        {
            return glm::vec2(p.x, p.z) * uvScale;
        };

        auto AddTriPT_WorldXZ = [&](std::vector<BuildingVertexPT>& v,
                                const glm::vec3& a,
                                const glm::vec3& b,
                                const glm::vec3& c)
        {
            v.push_back({a, UV_WorldXZ(a)});
            v.push_back({b, UV_WorldXZ(b)});
            v.push_back({c, UV_WorldXZ(c)});
        };

        auto AddQuadPT_WorldXZ = [&](std::vector<BuildingVertexPT>& v,
                                    const glm::vec3& a,
                                    const glm::vec3& b,
                                    const glm::vec3& c,
                                    const glm::vec3& d)
        {
            // a-b-c, a-c-d
            AddTriPT_WorldXZ(v, a, b, c);
            AddTriPT_WorldXZ(v, a, c, d);
        };

     
        const float inset = 0.06f;
        const float urbanParapetH = 0.10f;
        const float subParapetH   = 0.08f;
        const float ruralParapetH = 0.05f;
        const float roofEps = 0.01f;

        for (const auto& lot : lots.lots)
        {
            if (!lot.hasFootPrint) continue;
            if (lot.footprint.size() != 4) continue;

            auto& out = ZoneVec(lot.zone);

            const float h = std::max(1, lot.floors) * floorHeight;
            const float roofY = baseY + h;

            const glm::vec2 p0 = lot.footprint[0];
            const glm::vec2 p1 = lot.footprint[1];
            const glm::vec2 p2 = lot.footprint[2];
            const glm::vec2 p3 = lot.footprint[3];

            const glm::vec2 c2 = (p0 + p1 + p2 + p3) * 0.25f;

            if (lot.zone != road::LotZone::Rural)
            {
                float ph = ruralParapetH;
                if (lot.zone == road::LotZone::Urban) ph = urbanParapetH;
                else if (lot.zone == road::LotZone::Suburban) ph = subParapetH;

                glm::vec3 o0 = XZToVec3(p0, roofY);
                glm::vec3 o1 = XZToVec3(p1, roofY);
                glm::vec3 o2 = XZToVec3(p2, roofY);
                glm::vec3 o3 = XZToVec3(p3, roofY);

                glm::vec2 ip0 = InsetTowardCentroid(p0, c2, inset);
                glm::vec2 ip1 = InsetTowardCentroid(p1, c2, inset);
                glm::vec2 ip2 = InsetTowardCentroid(p2, c2, inset);
                glm::vec2 ip3 = InsetTowardCentroid(p3, c2, inset);

                glm::vec3 i0 = XZToVec3(ip0, roofY);
                glm::vec3 i1 = XZToVec3(ip1, roofY);
                glm::vec3 i2 = XZToVec3(ip2, roofY);
                glm::vec3 i3 = XZToVec3(ip3, roofY);

                glm::vec3 ot0 = o0 + glm::vec3(0, ph, 0);
                glm::vec3 ot1 = o1 + glm::vec3(0, ph, 0);
                glm::vec3 ot2 = o2 + glm::vec3(0, ph, 0);
                glm::vec3 ot3 = o3 + glm::vec3(0, ph, 0);

                glm::vec3 it0 = i0 + glm::vec3(0, ph, 0);
                glm::vec3 it1 = i1 + glm::vec3(0, ph, 0);
                glm::vec3 it2 = i2 + glm::vec3(0, ph, 0);
                glm::vec3 it3 = i3 + glm::vec3(0, ph, 0);

                // Parapet walls + cap
                AddQuadPT_WorldXZ(out, o0, o1, ot1, ot0);
                AddQuadPT_WorldXZ(out, o1, o2, ot2, ot1);
                AddQuadPT_WorldXZ(out, o2, o3, ot3, ot2);
                AddQuadPT_WorldXZ(out, o3, o0, ot0, ot3);

                AddQuadPT_WorldXZ(out, i1, i0, it0, it1);
                AddQuadPT_WorldXZ(out, i2, i1, it1, it2);
                AddQuadPT_WorldXZ(out, i3, i2, it2, it3);
                AddQuadPT_WorldXZ(out, i0, i3, it3, it0);

                AddQuadPT_WorldXZ(out, ot0, ot1, it1, it0);
                AddQuadPT_WorldXZ(out, ot1, ot2, it2, it1);
                AddQuadPT_WorldXZ(out, ot2, ot3, it3, it2);
                AddQuadPT_WorldXZ(out, ot3, ot0, it0, it3);

                // roof fill
                AddQuadPT_WorldXZ(out,
                                i0 + glm::vec3(0, roofEps, 0),
                                i1 + glm::vec3(0, roofEps, 0),
                                i2 + glm::vec3(0, roofEps, 0),
                                i3 + glm::vec3(0, roofEps, 0));
            }
            else
            {
                // Rural gable roof (textured)
                const float e01 = glm::length(p1 - p0);
                const float e12 = glm::length(p2 - p1);
                const bool ridgeParallelTo01 = (e01 >= e12);

                float shortSide = std::min(e01, e12);
                float ridgeH = glm::clamp(shortSide * 0.25f, 0.18f, 0.55f);

                if (ridgeParallelTo01)
                {
                    glm::vec2 m03 = (p0 + p3) * 0.5f;
                    glm::vec2 m12 = (p1 + p2) * 0.5f;

                    glm::vec3 rA = XZToVec3(m03, roofY + ridgeH);
                    glm::vec3 rB = XZToVec3(m12, roofY + ridgeH);

                    // slopes
                    AddQuadPT_WorldXZ(out, XZToVec3(p0, roofY), XZToVec3(p1, roofY), rB, rA);
                    AddQuadPT_WorldXZ(out, XZToVec3(p3, roofY), XZToVec3(p2, roofY), rB, rA);

                    // gable ends
                    AddTriPT_WorldXZ(out, XZToVec3(p0, roofY), rA, XZToVec3(p3, roofY));
                    AddTriPT_WorldXZ(out, XZToVec3(p1, roofY), XZToVec3(p2, roofY), rB);
                }
                else
                {
                    glm::vec2 m01 = (p0 + p1) * 0.5f;
                    glm::vec2 m32 = (p3 + p2) * 0.5f;

                    glm::vec3 rA = XZToVec3(m01, roofY + ridgeH);
                    glm::vec3 rB = XZToVec3(m32, roofY + ridgeH);

                    // slopes
                    AddQuadPT_WorldXZ(out, XZToVec3(p1, roofY), XZToVec3(p2, roofY), rB, rA);
                    AddQuadPT_WorldXZ(out, XZToVec3(p0, roofY), XZToVec3(p3, roofY), rB, rA);

                    // gable ends
                    AddTriPT_WorldXZ(out, XZToVec3(p1, roofY), rA, XZToVec3(p0, roofY));
                    AddTriPT_WorldXZ(out, XZToVec3(p2, roofY), XZToVec3(p3, roofY), rB);
                }
            }
        }
    }


    void BuildRoofDetailVerts(const LotCollection& lots,
                            std::vector<glm::vec3>& outRoofQuads,
                            std::vector<glm::vec3>& outRoofTris,
                            float baseY,
                            float floorHeight)
    {
        outRoofQuads.clear();
        outRoofTris.clear();

  
        const float inset = 0.06f;          // parapet thickness inward
        const float epsOut = 0.0015f;       // to avoid z-fighting
        const float urbanParapetH = 0.10f;
        const float subParapetH   = 0.08f;
        const float ruralParapetH = 0.05f;

        (void)epsOut; // currently unused

        for (const auto& lot : lots.lots)
        {
            if (!lot.hasFootPrint) continue;
            if (lot.footprint.size() != 4) continue;

            const float h = std::max(1, lot.floors) * floorHeight;
            const float roofY = baseY + h;

            const glm::vec2 p0 = lot.footprint[0];
            const glm::vec2 p1 = lot.footprint[1];
            const glm::vec2 p2 = lot.footprint[2];
            const glm::vec2 p3 = lot.footprint[3];

            const glm::vec2 c = (p0 + p1 + p2 + p3) * 0.25f;


            if (lot.zone != road::LotZone::Rural)
            {
                // Urban/Suburban parapet ring (quads)
                float ph = ruralParapetH;
                if (lot.zone == road::LotZone::Urban) ph = urbanParapetH;
                else if (lot.zone == road::LotZone::Suburban) ph = subParapetH;

                // Outer ring points at roof plane
                glm::vec3 o0 = XZToVec3(p0, roofY);
                glm::vec3 o1 = XZToVec3(p1, roofY);
                glm::vec3 o2 = XZToVec3(p2, roofY);
                glm::vec3 o3 = XZToVec3(p3, roofY);

                // Inner ring points inset toward centroid
                glm::vec2 ip0 = InsetTowardCentroid(p0, c, inset);
                glm::vec2 ip1 = InsetTowardCentroid(p1, c, inset);
                glm::vec2 ip2 = InsetTowardCentroid(p2, c, inset);
                glm::vec2 ip3 = InsetTowardCentroid(p3, c, inset);

                glm::vec3 i0 = XZToVec3(ip0, roofY);
                glm::vec3 i1 = XZToVec3(ip1, roofY);
                glm::vec3 i2 = XZToVec3(ip2, roofY);
                glm::vec3 i3 = XZToVec3(ip3, roofY);

                // top rings
                glm::vec3 ot0 = o0 + glm::vec3(0, ph, 0);
                glm::vec3 ot1 = o1 + glm::vec3(0, ph, 0);
                glm::vec3 ot2 = o2 + glm::vec3(0, ph, 0);
                glm::vec3 ot3 = o3 + glm::vec3(0, ph, 0);

                glm::vec3 it0 = i0 + glm::vec3(0, ph, 0);
                glm::vec3 it1 = i1 + glm::vec3(0, ph, 0);
                glm::vec3 it2 = i2 + glm::vec3(0, ph, 0);
                glm::vec3 it3 = i3 + glm::vec3(0, ph, 0);

                // Outer vertical walls
                AddQuad(outRoofQuads, o0, o1, ot1, ot0);
                AddQuad(outRoofQuads, o1, o2, ot2, ot1);
                AddQuad(outRoofQuads, o2, o3, ot3, ot2);
                AddQuad(outRoofQuads, o3, o0, ot0, ot3);

                // Inner vertical walls (flip winding-ish)
                AddQuad(outRoofQuads, i1, i0, it0, it1);
                AddQuad(outRoofQuads, i2, i1, it1, it2);
                AddQuad(outRoofQuads, i3, i2, it2, it3);
                AddQuad(outRoofQuads, i0, i3, it3, it0);

                // Top cap ring (horizontal)
                AddQuad(outRoofQuads, ot0, ot1, it1, it0);
                AddQuad(outRoofQuads, ot1, ot2, it2, it1);
                AddQuad(outRoofQuads, ot2, ot3, it3, it2);
                AddQuad(outRoofQuads, ot3, ot0, it0, it3);

                // flat inner fill (slightly above roof plane)
                const float roofEps = 0.0015f;
                AddQuad(outRoofQuads,
                        i0 + glm::vec3(0, roofEps, 0),
                        i1 + glm::vec3(0, roofEps, 0),
                        i2 + glm::vec3(0, roofEps, 0),
                        i3 + glm::vec3(0, roofEps, 0));
            }
            else
            {
                // Rural: gable roof (2 slope quads + 2 end tris)
                const float e01 = glm::length(p1 - p0);
                const float e12 = glm::length(p2 - p1);
                const bool ridgeParallelTo01 = (e01 >= e12);

                float shortSide = std::min(e01, e12);
                float ridgeH = glm::clamp(shortSide * 0.25f, 0.18f, 0.55f);

                if (ridgeParallelTo01)
                {
                    // Ridge parallel to edge 0-1 => ridge endpoints on edges 0-3 and 1-2
                    glm::vec2 m03 = (p0 + p3) * 0.5f;
                    glm::vec2 m12 = (p1 + p2) * 0.5f;

                    glm::vec3 rA = XZToVec3(m03, roofY + ridgeH);
                    glm::vec3 rB = XZToVec3(m12, roofY + ridgeH);

                    // slope planes
                    AddQuad(outRoofQuads, XZToVec3(p0, roofY), XZToVec3(p1, roofY), rB, rA);
                    AddQuad(outRoofQuads, XZToVec3(p3, roofY), XZToVec3(p2, roofY), rB, rA);

                    // end caps (gable ends)
                    AddTri(outRoofTris, XZToVec3(p0, roofY), rA, XZToVec3(p3, roofY));
                    AddTri(outRoofTris, XZToVec3(p1, roofY), XZToVec3(p2, roofY), rB);
                }
                else
                {
                    // Ridge parallel to edge 1-2 => ridge endpoints on edges 0-1 and 3-2
                    glm::vec2 m01 = (p0 + p1) * 0.5f;
                    glm::vec2 m32 = (p3 + p2) * 0.5f;

                    glm::vec3 rA = XZToVec3(m01, roofY + ridgeH);
                    glm::vec3 rB = XZToVec3(m32, roofY + ridgeH);

                    // slope planes
                    AddQuad(outRoofQuads, XZToVec3(p1, roofY), XZToVec3(p2, roofY), rB, rA);
                    AddQuad(outRoofQuads, XZToVec3(p0, roofY), XZToVec3(p3, roofY), rB, rA);

                    // end caps (gable ends)
                    AddTri(outRoofTris, XZToVec3(p1, roofY), rA, XZToVec3(p0, roofY));
                    AddTri(outRoofTris, XZToVec3(p2, roofY), XZToVec3(p3, roofY), rB);
                }
            }
        }
    }



    void BuildWindowDetailQuads(const LotCollection& lots,
                                    std::vector<glm::vec3>& outWindowQuads,
                                    float baseY,
                                    float floorHeight)
    {
        outWindowQuads.clear();

        const float windowInsetFromEdges = 0.18f; // margins on wall
        const float windowW = 0.22f;
        const float windowH = 0.22f;
        const float wallEps = 0.01f;             // offset from wall to avoid z-fighting
        const float sillY = 0.08f;                // from floor base

        for (const auto& lot : lots.lots)
        {
            if (!lot.hasFootPrint) continue;
            if (lot.footprint.size() != 4) continue;

            const int floors = std::max(1, lot.floors);
            const float buildingH = floors * floorHeight;

            // density by zone
            int colsMin = 2, colsMax = 5;
            if (lot.zone == road::LotZone::Urban)    { colsMin = 4; colsMax = 9; }
            if (lot.zone == road::LotZone::Suburban) { colsMin = 3; colsMax = 7; }
            if (lot.zone == road::LotZone::Rural)    { colsMin = 1; colsMax = 6; }

            const glm::vec2 p[4] = { lot.footprint[0], lot.footprint[1], lot.footprint[2], lot.footprint[3] };

            // determine winding so “outward” is correct
            std::vector<glm::vec2> poly = {p[0],p[1],p[2],p[3]};
            const bool ccw = (SignedArea2(poly) > 0.0f);

            for (int e = 0; e < 4; ++e)
            {
                const glm::vec2 a2 = p[e];
                const glm::vec2 b2 = p[(e+1)%4];
                const glm::vec2 edge = b2 - a2;
                const float edgeLen = glm::length(edge);
                if (edgeLen < 0.25f) continue;

                glm::vec2 t = edge / edgeLen; // tangent along wall
                glm::vec2 n = ccw ? glm::vec2(t.y, -t.x) : glm::vec2(-t.y, t.x); // outward

                // decide columns based on edge length
                int cols = 0;

                if (lot.zone == road::LotZone::Rural)
                {

                    float desiredSpacing = 1.2f;
                    cols = (int)std::floor(edgeLen / desiredSpacing);
                    cols = glm::clamp(cols, colsMin, colsMax);
                }
                else
                {
                    cols = (int)glm::clamp(edgeLen / 0.6f, (float)colsMin, (float)colsMax);
                }

                float margin = windowInsetFromEdges;

                if (lot.zone == road::LotZone::Rural)
                {

                        // deterministic pseudo-random from lotId + edge index
                        uint32_t h = (uint32_t)(lot.lotId * 73856093u) ^ (uint32_t)(e * 19349663u);
                        float r = (float)(h & 0xFFFFu) / 65535.0f;

                        // base rural margin range
                        float m = 0.10f + r * 0.14f; // 0.10 .. 0.24

                        // cap margin so long walls don't look empty
                        float maxMargin = std::min(0.30f, edgeLen * 0.12f); // <= 12% of wall, max 0.30m
                        margin = std::min(m, maxMargin);
                    
                }

                float usable = edgeLen - 2.0f * margin;
                if (usable < windowW) continue;

                float step = usable / (float)cols;

                int floorsForWindows = floors;
                if (lot.zone == road::LotZone::Rural)
                    floorsForWindows = std::min(floors, 2); // rural: single row only

                // If rural has 2+ floors, force 2 rows
                if (lot.zone == road::LotZone::Rural && floors >= 2)
                    floorsForWindows = 2;


                for (int f = 0; f < floorsForWindows; ++f)
                {

                    float y0 = baseY + f * floorHeight + sillY;
                    float y1 = y0 + windowH;

                    for (int c = 0; c < cols; ++c)
                    {

                        float center = margin + (c + 0.5f) * step;
                        float u0 = center - windowW * 0.5f;
                        float u1 = center + windowW * 0.5f;

                        glm::vec2 wA = a2 + t * u0;
                        glm::vec2 wB = a2 + t * u1;

                        glm::vec3 bl = XZToVec3(wA, y0) + glm::vec3(n.x, 0, n.y) * wallEps;
                        glm::vec3 br = XZToVec3(wB, y0) + glm::vec3(n.x, 0, n.y) * wallEps;
                        glm::vec3 tr = XZToVec3(wB, y1) + glm::vec3(n.x, 0, n.y) * wallEps;
                        glm::vec3 tl = XZToVec3(wA, y1) + glm::vec3(n.x, 0, n.y) * wallEps;

                        AddQuad(outWindowQuads, bl, br, tr, tl);
                    }
                }
            }
        }
    }
}
