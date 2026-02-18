#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader;

namespace road
{
    class LotCollection;

    struct BuildingVertexPT
    {
        glm::vec3 pos{0.0f};
        glm::vec2 uv{0.0f};
    };

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

        // draws triangle meshes from a list of positions+uvs.
    class BuildingTexturedRenderer
    {
    private:
        GLuint mVAO = 0;
        GLuint mVBO = 0;
        std::size_t mVertexCount = 0;

    public:
        bool Initialize();
        void Shutdown();

        void Upload(const std::vector<BuildingVertexPT>& triVerts);

        // textureId may be 0, in which case uUseTexture should be false.
        void Draw(const Shader& shader,
                  const glm::mat4& vp,
                  GLuint textureId,
                  bool useTexture,
                  const glm::vec3& fallbackColor) const;

        std::size_t VertexCount() const { return mVertexCount; }
    };

    // builds a simple extruded mesh from lot footprints: walls (4 sides), roof (2 triangles)
    void BuildBuildingTriVerts(const LotCollection& lots,
                               std::vector<glm::vec3>& outTriVerts,
                               float baseY,
                               float floorHeight);
    //build mesh with quads only
    void BuildBuildingQuadVerts(const LotCollection& lots,
                                std::vector<glm::vec3>& outQuadVerts,
                                float baseY,
                                float floorHeight);

    void BuildBuildingTriVertsTexturedByZone(const LotCollection& lots,
                                            std::vector<BuildingVertexPT>& outUrban,
                                            std::vector<BuildingVertexPT>& outSuburban,
                                            std::vector<BuildingVertexPT>& outRural,
                                            float baseY,
                                            float floorHeight,
                                            float uvMetersPerTile = 2.0f);
    // Roof details
    void BuildRoofDetailVerts(const LotCollection& lots,
                            std::vector<glm::vec3>& outRoofQuads,
                            std::vector<glm::vec3>& outRoofTris,
                            float baseY,
                            float floorHeight);

    // Roof details texture split by zone
    void BuildRoofDetailTriVertsTexturedByZone(const LotCollection& lots,
                                            std::vector<BuildingVertexPT>& outUrban,
                                            std::vector<BuildingVertexPT>& outSuburban,
                                            std::vector<BuildingVertexPT>& outRural,
                                            float baseY,
                                            float floorHeight,
                                            float uvMetersPerTile = 2.0f);


    // Windows 
    void BuildWindowDetailQuads(const LotCollection& lots,
                                std::vector<glm::vec3>& outWindowQuads,
                                float baseY,
                                float floorHeight);

    
}
