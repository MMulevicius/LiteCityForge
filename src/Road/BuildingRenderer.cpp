#include "Road/BuildingRenderer.h"
#include "Road/LotSubdivision.h"   
#include <glm/gtc/type_ptr.hpp>
#include "Shader.h"
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

}
