#include "Road/BuildingRenderer.h"
#include "Road/LotSubdivision.h"   
#include <glm/gtc/type_ptr.hpp>
#include "Shader.h"

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
}
