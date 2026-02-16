#include "Export/CityExporter.h"
#include "Export/ObjWriter.h"
#include <filesystem>

namespace export3d
{
    static std::vector<glm::vec3> MakeGroundTris(const glm::vec2& centerXZ, float halfW, float halfH)
    {
        float x0 = centerXZ.x - halfW;
        float x1 = centerXZ.x + halfW;
        float z0 = centerXZ.y - halfH;
        float z1 = centerXZ.y + halfH;

        // two triangles, y = 0
        std::vector<glm::vec3> g;
        g.reserve(6);

        g.push_back({x0, 0.f, z0});
        g.push_back({x1, 0.f, z0});
        g.push_back({x1, 0.f, z1});

        g.push_back({x0, 0.f, z0});
        g.push_back({x1, 0.f, z1});
        g.push_back({x0, 0.f, z1});

        return g;
    }

    bool CityExporter::ExportOBJ(const std::string& outDir,
                                const std::string& baseName,
                                const CityExportInput& in,
                                std::string* outObjPath)
    {
        namespace fs = std::filesystem;

        const std::string objPath = (fs::path(outDir) / (baseName + ".obj")).string();
        const std::string mtlPath = (fs::path(outDir) / (baseName + ".mtl")).string();

        if (outObjPath) *outObjPath = objPath;

        try
        {
            fs::create_directories(outDir);
        }
        catch (...)
        {
            return false;
        }

        // materials
        std::vector<ObjMaterial> mats = {
            {"mat_ground",   {0.20f, 0.30f, 0.30f}},
            {"mat_hwy",      {0.07f, 0.07f, 0.07f}},
            {"mat_street",   {0.12f, 0.12f, 0.12f}},
            {"mat_sidewalk", {0.70f, 0.70f, 0.70f}},
            {"mat_building", {0.75f, 0.75f, 0.78f}},
            {"mat_roof",     {0.60f, 0.60f, 0.62f}},
            {"mat_window",   {0.25f, 0.40f, 0.55f}}
        };

        if (!ObjWriter::WriteMtl(mtlPath, mats))
            return false;

        std::vector<ObjMesh> meshes;
        meshes.push_back({"Ground", "mat_ground", MakeGroundTris(in.centerXZ, in.halfW, in.halfH)});
        meshes.push_back({"Roads_Highway", "mat_hwy", in.roadHighwayTris});
        meshes.push_back({"Roads_Street", "mat_street", in.roadStreetTris});
        meshes.push_back({"Sidewalks", "mat_sidewalk", in.sidewalkTris});
        //meshes.push_back({"Buildings", "mat_building", {}, in.buildingQuads});
        meshes.push_back({"Buildings_Base", "mat_building", {}, in.buildingQuads});

        if (!in.buildingRoofQuads.empty())
            meshes.push_back({"Buildings_Roof", "mat_roof", in.buildingRoofTris, in.buildingRoofQuads});
        if (!in.buildingWindowQuads.empty())
            meshes.push_back({"Buildings_Windows", "mat_window", {}, in.buildingWindowQuads});

        const std::string mtlFileName = baseName + ".mtl"; 
        if (!ObjWriter::WriteObj(objPath, mtlFileName, meshes))
            return false;

        if (outObjPath) *outObjPath = objPath;
        return true;
    }
}
