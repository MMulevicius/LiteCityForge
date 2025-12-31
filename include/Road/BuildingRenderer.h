#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader;

namespace road
{
    class LotCollection;

    // draws triangle meshes from a list of positions.
    class BuildingRenderer
    {
    private:
        GLuint mVAO = 0;
        GLuint mVBO = 0;
        std::size_t mVertexCount = 0;

    public:
        bool Initialize();
        void Shutdown();

        void Upload(const std::vector<glm::vec3>& triVerts);
        void Draw(const Shader& shader, const glm::mat4& vp) const;

        std::size_t VertexCount() const { return mVertexCount; }
    };

    // builds a simple extruded mesh from lot footprints: walls (4 sides), roof (2 triangles)
    void BuildBuildingTriVerts(const LotCollection& lots,
                               std::vector<glm::vec3>& outTriVerts,
                               float baseY,
                               float floorHeight);
}
