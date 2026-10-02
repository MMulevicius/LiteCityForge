#include "Export/CityExportController.h"

#include "Export/CityExporter.h"
#include "GUI/Gui.h"
#include "Generation/CityGenerationState.h"
#include "Rendering/RenderContexts.h"

#include <vector>

namespace export3d
{
    namespace
    {
        std::vector<glm::vec3> StripBuildingTopCapQuads(
            const std::vector<glm::vec3> &buildingQuadVerts)
        {
            std::vector<glm::vec3> out;
            out.reserve(buildingQuadVerts.size());

            const size_t vertsPerBuilding = 5 * 4;
            const size_t keepVerts = 4 * 4;

            for (size_t i = 0;
                 i + vertsPerBuilding <= buildingQuadVerts.size();
                 i += vertsPerBuilding)
            {
                out.insert(
                    out.end(),
                    buildingQuadVerts.begin() + i,
                    buildingQuadVerts.begin() + i + keepVerts);
            }

            return out;
        }
    }

    void HandleExportIfRequested(
        Gui &gui,
        const CityContext &city,
        const GroundContext &ground,
        const RoadContext &roads,
        const BuildingContext &buildings)
    {
        std::string dir;
        std::string base;
        Gui::ExportFormat format;

        if (!gui.ConsumeExportRequest(dir, base, format))
            return;

        CityExportInput exportInput;
        exportInput.centerXZ = city.centerXZ;
        exportInput.halfW = ground.halfW;
        exportInput.halfH = ground.halfH;

        exportInput.roadHighwayTris = roads.roadHighwayTris;
        exportInput.roadStreetTris = roads.roadStreetTris;
        exportInput.sidewalkTris = roads.sidewalkTris;
        exportInput.buildingTris = buildings.buildingTriVerts;

        if (gui.ExportBuildingRoofs())
        {
            exportInput.buildingQuads =
                StripBuildingTopCapQuads(buildings.buildingQuadVerts);
            exportInput.buildingRoofQuads = buildings.buildingRoofQuadVerts;
            exportInput.buildingRoofTris = buildings.buildingRoofTriVerts;
        }
        else
        {
            exportInput.buildingQuads = buildings.buildingQuadVerts;
            exportInput.buildingRoofQuads.clear();
            exportInput.buildingRoofTris.clear();
        }

        if (gui.ExportBuildingWindows())
            exportInput.buildingWindowQuads = buildings.buildingWindowQuadVerts;
        else
            exportInput.buildingWindowQuads.clear();

        std::string outPath;
        bool success = false;

        if (format == Gui::ExportFormat::OBJ)
        {
            success = CityExporter::ExportOBJ(
                dir, base, exportInput, &outPath);
        }

        gui.SetLastExportResult(success, outPath);
    }
}