#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace export3d
{
    //data for exporting
    struct CityExportInput
    {
        //city export bounds
        glm::vec2 centerXZ = {0,0};
        float halfW = 50.0f;
        float halfH = 50.0f;

        //geometry payload: roads/sidewalks/buildings
        std::vector<glm::vec3> roadHighwayTris;
        std::vector<glm::vec3> roadStreetTris;
        std::vector<glm::vec3> sidewalkTris;
        std::vector<glm::vec3> buildingTris;
        std::vector<glm::vec3> buildingQuads;

        //roofs + windows
        std::vector<glm::vec3> buildingRoofQuads;
        std::vector<glm::vec3> buildingRoofTris;
        std::vector<glm::vec3> buildingWindowQuads;
    };

    //provides a stateless OBJ/MTL
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
