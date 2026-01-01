#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace export3d
{
    struct CityExportInput
    {
        glm::vec2 centerXZ = {0,0};
        float halfW = 50.0f;
        float halfH = 50.0f;

        std::vector<glm::vec3> roadHighwayTris;
        std::vector<glm::vec3> roadStreetTris;
        std::vector<glm::vec3> sidewalkTris;
        std::vector<glm::vec3> buildingTris;
    };

    class CityExporter
    {
    public:
        // writes Exports/<baseName>.obj and .mtl
        static bool ExportOBJ(const std::string& outDir,
                              const std::string& baseName,
                              const CityExportInput& in,
                              std::string* outObjPath = nullptr);
    };
}
