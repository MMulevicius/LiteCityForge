#include "Export/CityExporter.h"
#include "Export/ObjWriter.h"
#include <filesystem>

namespace export3d
{
    //generates flat vector of vertices
    static std::vector<glm::vec3> MakeGroundTris(const glm::vec2& centerXZ, float halfW, float halfH)
    {
        //ground rectangle corners
        float x0 = centerXZ.x - halfW;
        float x1 = centerXZ.x + halfW;
        float z0 = centerXZ.y - halfH;
        float z1 = centerXZ.y + halfH;

        // two triangles, y = 0
        std::vector<glm::vec3> g;
        g.reserve(6);

        //splits quad into triangles
        g.push_back({x0, 0.f, z0});
        g.push_back({x1, 0.f, z0});
        g.push_back({x1, 0.f, z1});

        g.push_back({x0, 0.f, z0});
        g.push_back({x1, 0.f, z1});
        g.push_back({x0, 0.f, z1});

        return g;
    }

    // makes sure directory exists
    // decided material list
    // decide mesh groups (OBJ groups)
    // call ObjWrite to write .mtl and .obj
    bool CityExporter::ExportOBJ(const std::string& outDir,
                                const std::string& baseName,
                                const CityExportInput& in,
                                std::string* outObjPath)
    {
        namespace fs = std::filesystem;

        if (outDir.empty() || baseName.empty())
            return false;

        // expand "~" to HOME on Linux/macOS.
        auto ExpandTilde = [](const std::string& s) -> std::string
        {
            if (!s.empty() && s[0] == '~')
            {
                const char* home = std::getenv("HOME");
                if (home && home[0] != '\0')
                {
                    if (s.size() == 1) return std::string(home);
                    if (s[1] == '/')   return std::string(home) + s.substr(1); // keep the '/'
                }
            }
            return s;
        };
        //handle file path
        fs::path dirPath = fs::path(ExpandTilde(outDir));

        if (dirPath.has_extension())
            dirPath = dirPath.parent_path();

        // create output directory (no exceptions)
        std::error_code ec;
        fs::create_directories(dirPath, ec);
        if (ec || dirPath.empty())
            return false;

        // construct final output paths
        const fs::path objPathP = dirPath / (baseName + ".obj");
        const fs::path mtlPathP = dirPath / (baseName + ".mtl");

        if (outObjPath) *outObjPath = objPathP.string();

        // materials (MTL content)
        std::vector<ObjMaterial> mats = {
            {"mat_ground",   {0.20f, 0.30f, 0.30f}},
            {"mat_hwy",      {0.07f, 0.07f, 0.07f}},
            {"mat_street",   {0.12f, 0.12f, 0.12f}},
            {"mat_sidewalk", {0.70f, 0.70f, 0.70f}},
            {"mat_building", {0.75f, 0.75f, 0.78f}},
            {"mat_roof",     {0.60f, 0.60f, 0.62f}},
            {"mat_window",   {0.25f, 0.40f, 0.55f}}
        };

        //write MTL first
        if (!ObjWriter::WriteMtl(mtlPathP.string(), mats))
            return false;

        //build mesh groups
        std::vector<ObjMesh> meshes;

        //ground + roads + sidewalk + building nase
        meshes.push_back({"Ground", "mat_ground", MakeGroundTris(in.centerXZ, in.halfW, in.halfH)});
        meshes.push_back({"Roads_Highway", "mat_hwy", in.roadHighwayTris});
        meshes.push_back({"Roads_Street", "mat_street", in.roadStreetTris});
        meshes.push_back({"Sidewalks", "mat_sidewalk", in.sidewalkTris});
        meshes.push_back({"Buildings_Base", "mat_building", {}, in.buildingQuads});

        // adds roofs and windows if toggled on
        if (!in.buildingRoofQuads.empty())
            meshes.push_back({"Buildings_Roof", "mat_roof", in.buildingRoofTris, in.buildingRoofQuads});
        if (!in.buildingWindowQuads.empty())
            meshes.push_back({"Buildings_Windows", "mat_window", {}, in.buildingWindowQuads});

        // write obj
        const std::string mtlFileName = baseName + ".mtl";
        if (!ObjWriter::WriteObj(objPathP.string(), mtlFileName, meshes))
            return false;

        if (outObjPath) *outObjPath = objPathP.string();
        return true;
    }

}
