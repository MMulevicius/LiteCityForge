#include "Generation/CityGenerator.h"
#include "Road/RoadSurfaceGenerator.h"
#include "Road/SideWalkGenerator.h"

#include <chrono>
#include <iostream>
#include <algorithm>

namespace generation
{
    namespace
    {
        // prints the current road-generation parameters for debugging
        void PrintRoadParams(const road::RoadParams &rp)
        {
            std::cout
                << "[GUI] cityRadius=" << rp.cityRadius
                << " maxIterations=" << rp.maxIterations
                << " maxStreets=" << rp.maxStreetSegments
                << " maxHighways=" << rp.maxHighwaySegments
                << "\n";
        }

        // converts plain triangle vertices into textured vertices using world-space XZ UV mapping
        void ConvertTriVertsToPT_WorldXZ(const std::vector<glm::vec3> &in,
                                         std::vector<road::BuildingVertexPT> &out,
                                         float uvMetersPerTile)
        {
            out.clear();
            out.reserve(in.size());

            if (uvMetersPerTile <= 0.001f)
                uvMetersPerTile = 2.0f;
            const float s = 1.0f / uvMetersPerTile;

            for (const auto &p : in)
            {
                road::BuildingVertexPT v;
                v.pos = p;
                v.uv = glm::vec2(p.x, p.z) * s;
                out.push_back(v);
            }
        }

        // build road and sidewalk surface meshes then uploads them to GPU renderers
        void BuildAndUploadRoadAndSidewalkMeshes(
            const road::RoadNetwork &roadNet,
            const road::RoadParams &roadParams,
            std::vector<glm::vec3> &roadHighwayTris,
            std::vector<glm::vec3> &roadStreetTris,
            std::vector<glm::vec3> &sidewalkTris,
            road::BuildingRenderer &roadMeshHighway,
            road::BuildingRenderer &roadMeshStreet,
            road::BuildingRenderer &sidewalkMesh)
        {
            // build 3D road + sidewalk slabs
            const float roadBaseY = 0.02f;
            const float roadH = 0.02f;

            // sidewalk sits above road
            const float sidewalkBaseY = roadBaseY + roadH;
            const float sidewalkH = 0.08f;

            // trim
            const float trimExtra = 0.0f;

            road::BuildRoadSurfaceTriVerts(
                roadNet, roadParams,
                roadHighwayTris, roadStreetTris,
                roadBaseY, roadH);

            road::BuildSidewalkSurfaceTriVerts(
                roadNet, roadParams,
                sidewalkTris,
                sidewalkBaseY, sidewalkH,
                trimExtra, false);

            roadMeshHighway.Upload(roadHighwayTris);
            roadMeshStreet.Upload(roadStreetTris);
            sidewalkMesh.Upload(sidewalkTris);

            std::cout << "Road tris => highway: " << roadHighwayTris.size()
                      << " street: " << roadStreetTris.size()
                      << " sidewalks: " << sidewalkTris.size()
                      << "\n";
        }

        // builds and uploads debug line geometry for highways and streets
        void BuildAndUploadRoadLineVerts(const road::RoadNetwork &roadNet, road::LineRenderer &highwayLines,
                                         road::LineRenderer &streetLines)
        {
            std::vector<glm::vec3> highwayVerts;
            std::vector<glm::vec3> streetVerts;

            highwayVerts.reserve(roadNet.Segments().size() * 2);
            streetVerts.reserve(roadNet.Segments().size() * 2);

            for (const auto &seg : roadNet.Segments())
            {
                const auto &A = roadNet.Nodes().at(seg.a - 1).pos;
                const auto &B = roadNet.Nodes().at(seg.b - 1).pos;

                glm::vec3 a3(A.x, 0.05f, A.y);
                glm::vec3 b3(B.x, 0.05f, B.y);

                if (seg.type == road::RoadType::Highway)
                {
                    highwayVerts.push_back(a3);
                    highwayVerts.push_back(b3);
                }
                else
                {
                    streetVerts.push_back(a3);
                    streetVerts.push_back(b3);
                }
            }
            highwayLines.Upload(highwayVerts);
            streetLines.Upload(streetVerts);
        }

        // derives lot generation parameters from the current GUI and road settings
        road::LotParams MakeLotParamsFromRoadParams(const Gui &gui, const road::RoadParams &roadParams)
        {
            road::LotParams lotParams = gui.GetLotParams();
            lotParams.seed = roadParams.seed;
            lotParams.cityRadius = roadParams.cityRadius;

            lotParams.streetHalfWidth = roadParams.streetHalfWidth;
            lotParams.highwayHalfWidth = roadParams.highwayHalfWidth;

            lotParams.sidewalkWidth = roadParams.sidewalkWidth;
            lotParams.sidewalkGap = roadParams.sidewalkGap;

            return lotParams;
        }

        // prints lot zoning and garden-generation statistics for debugging
        void PrintGardenPipelineStats(const road::LotCollection lots, const road::LotParams &lotParams)
        {

            int nUrban = 0, nSub = 0, nRural = 0;
            int passZone = 0, passArea = 0, passDepth = 0, passAll = 0;

            for (const auto &l : lots.lots)
            {
                if (l.zone == road::LotZone::Urban)
                    nUrban++;
                else if (l.zone == road::LotZone::Suburban)
                    nSub++;
                else
                    nRural++;

                // zone
                if (l.zone != road::LotZone::Suburban)
                    continue;
                passZone++;

                // area
                if (l.area < lotParams.minLotAreaForGarden)
                    continue;
                passArea++;

                // depth feasibility (recompute from boundary)
                if (l.boundary.size() < 4)
                    continue;
                float fullDepth = glm::length(l.boundary[3] - l.boundary[0]);
                float gardenDepth = fullDepth * lotParams.gardenBackRatio;

                if (gardenDepth < lotParams.minGardenDepth)
                    continue;
                if ((fullDepth - gardenDepth) < 0.5f)
                    continue;
                passDepth++;

                // actual result
                if (l.hasGarden)
                    passAll++;
            }

            std::cout << "Zones => Urban: " << nUrban << " Suburban: " << nSub << " Rural: " << nRural << "\n";
            std::cout << "Garden pipeline => passZone: " << passZone
                      << " passArea: " << passArea
                      << " passDepth: " << passDepth
                      << " hasGarden: " << passAll << "\n";

            float minA = 1e9f, maxA = 0.0f;
            for (const auto &l : lots.lots)
            {
                minA = std::min(minA, l.area);
                maxA = std::max(maxA, l.area);
            }
            std::cout << "Lot area range: min=" << minA << " max=" << maxA
                      << " (threshold=" << lotParams.minLotAreaForGarden << ")\n";
        }

        // builds and uploads all lot-derived geometry:
        // lots, sidewalks, gardens, footprints, buildings,roofs and windows
        void BuildAndUploadLotDerivedGeometry(
            RoadContext &roads,
            LotContext &lotState,
            BuildingContext &buildings,
            TextureContext &textures)
        {

            const float baseY = 0.03f;
            const float floorH = 0.35f;

            // build debug line geometry for lot-related overlays
            road::BuildLotLineVerts(lotState.lots, lotState.lotLineVerts, 0.02f);
            road::BuildSidewalkLineVerts(roads.roadNet, roads.roadParams, roads.sidewalkLineVerts, 0.06f);
            road::BuildGardenLineVerts(lotState.lots, lotState.gardenLineVerts, 0.021f);
            road::BuildFootprintLineVerts(lotState.lots, lotState.footprintLineVerts, 0.022f);

            // triangles for rendering
            road::BuildBuildingTriVerts(lotState.lots, buildings.buildingTriVerts, baseY, floorH);

            // quads for exporting
            road::BuildBuildingQuadVerts(lotState.lots, buildings.buildingQuadVerts, baseY, floorH);

            road::BuildBuildingTriVertsTexturedByZone(lotState.lots, textures.buildingUrbanPT, textures.buildingSuburbanPT, textures.buildingRuralPT,
                                                      baseY, floorH, 2.0f);

            // roof details
            road::BuildRoofDetailVerts(lotState.lots, buildings.buildingRoofQuadVerts, buildings.buildingRoofTriVerts, baseY, floorH);

            road::BuildRoofDetailTriVertsTexturedByZone(
                lotState.lots,
                textures.roofUrbanPT, textures.roofSuburbanPT, textures.roofRuralPT,
                baseY, floorH,
                2.0f);

            textures.roofMeshUrban.Upload(textures.roofUrbanPT);
            textures.roofMeshSuburban.Upload(textures.roofSuburbanPT);
            textures.roofMeshRural.Upload(textures.roofRuralPT);

            // windows (quads)
            road::BuildWindowDetailQuads(lotState.lots, buildings.buildingWindowQuadVerts, baseY, floorH);

            // convert window quads -> tris for rendering with BuildingRenderer
            buildings.buildingWindowTriVerts.clear();
            buildings.buildingWindowTriVerts.reserve((buildings.buildingWindowQuadVerts.size() / 4) * 6);
            for (size_t i = 0; i + 3 < buildings.buildingWindowQuadVerts.size(); i += 4)
            {
                const auto &a = buildings.buildingWindowQuadVerts[i + 0];
                const auto &b = buildings.buildingWindowQuadVerts[i + 1];
                const auto &c = buildings.buildingWindowQuadVerts[i + 2];
                const auto &d = buildings.buildingWindowQuadVerts[i + 3];
                // two tris
                buildings.buildingWindowTriVerts.push_back(a);
                buildings.buildingWindowTriVerts.push_back(b);
                buildings.buildingWindowTriVerts.push_back(c);
                buildings.buildingWindowTriVerts.push_back(a);
                buildings.buildingWindowTriVerts.push_back(c);
                buildings.buildingWindowTriVerts.push_back(d);
            }

            std::vector<road::BuildingVertexPT> windowPTTris;
            windowPTTris.reserve((buildings.buildingWindowQuadVerts.size() / 4) * 6);

            for (size_t i = 0; i + 3 < buildings.buildingWindowQuadVerts.size(); i += 4)
            {
                const glm::vec3 &a = buildings.buildingWindowQuadVerts[i + 0];
                const glm::vec3 &b = buildings.buildingWindowQuadVerts[i + 1];
                const glm::vec3 &c = buildings.buildingWindowQuadVerts[i + 2];
                const glm::vec3 &d = buildings.buildingWindowQuadVerts[i + 3];

                // UVs (simple quad mapping)
                road::BuildingVertexPT A{a, glm::vec2(0.0f, 0.0f)};
                road::BuildingVertexPT B{b, glm::vec2(1.0f, 0.0f)};
                road::BuildingVertexPT C{c, glm::vec2(1.0f, 1.0f)};
                road::BuildingVertexPT D{d, glm::vec2(0.0f, 1.0f)};

                // two tris
                windowPTTris.push_back(A);
                windowPTTris.push_back(B);
                windowPTTris.push_back(C);

                windowPTTris.push_back(A);
                windowPTTris.push_back(C);
                windowPTTris.push_back(D);
            }

            std::vector<glm::vec3> roofRenderTris;
            roofRenderTris.reserve((buildings.buildingRoofQuadVerts.size() / 4) * 6 + buildings.buildingRoofTriVerts.size());

            // convert roof quads -> tris
            for (size_t i = 0; i + 3 < buildings.buildingRoofQuadVerts.size(); i += 4)
            {
                const glm::vec3 &a = buildings.buildingRoofQuadVerts[i + 0];
                const glm::vec3 &b = buildings.buildingRoofQuadVerts[i + 1];
                const glm::vec3 &c = buildings.buildingRoofQuadVerts[i + 2];
                const glm::vec3 &d = buildings.buildingRoofQuadVerts[i + 3];

                // two triangles: a-b-c and a-c-d
                roofRenderTris.push_back(a);
                roofRenderTris.push_back(b);
                roofRenderTris.push_back(c);

                roofRenderTris.push_back(a);
                roofRenderTris.push_back(c);
                roofRenderTris.push_back(d);
            }

            // append any roof tris (rural gable end caps )
            roofRenderTris.insert(roofRenderTris.end(), buildings.buildingRoofTriVerts.begin(), buildings.buildingRoofTriVerts.end());

            std::cout << "Building tri verts: " << buildings.buildingTriVerts.size() << "\n";
            std::cout << "Building quad verts: " << buildings.buildingQuadVerts.size() << "\n";
            lotState.lotLines.Upload(lotState.lotLineVerts);
            roads.sidewalkLines.Upload(roads.sidewalkLineVerts);
            lotState.gardenLines.Upload(lotState.gardenLineVerts);
            lotState.footprintLines.Upload(lotState.footprintLineVerts);
            buildings.buildingMesh.Upload(buildings.buildingTriVerts);
            textures.buildingMeshUrban.Upload(textures.buildingUrbanPT);
            textures.buildingMeshSuburban.Upload(textures.buildingSuburbanPT);
            textures.buildingMeshRural.Upload(textures.buildingRuralPT);
            buildings.roofMesh.Upload(roofRenderTris);
            buildings.windowMesh.Upload(buildings.buildingWindowTriVerts);
            textures.windowMeshTex.Upload(windowPTTris);

            std::cout << "Lots count: " << lotState.lots.lots.size() << "\n";
            std::cout << "Lot verts: " << lotState.lotLineVerts.size() << "\n";
            std::cout << "Garden verts: " << lotState.gardenLineVerts.size() << "\n";
        }
    }

    // runs the full city generation pipeline and uploads all resulting geometry
    void GenerateCityNow(
        Gui &gui,
        RoadContext &roads,
        LotContext &lotState,
        BuildingContext &buildings,
        TextureContext &textures)
    {

        // pull params from GUI
        roads.roadParams = gui.GetParams();
        PrintRoadParams(roads.roadParams);

        // start timer
        using Clock = std::chrono::high_resolution_clock;
        auto t0 = Clock::now();

        // generate road network
        roads.roadNet = roads.roadGen.Generate(roads.roadParams);

        // build & upload road + sidewalk slabs
        BuildAndUploadRoadAndSidewalkMeshes(
            roads.roadNet, roads.roadParams,
            roads.roadHighwayTris, roads.roadStreetTris, roads.sidewalkTris,
            roads.roadMeshHighway, roads.roadMeshStreet, roads.sidewalkMesh);

        // build PT buffers (UVs) + upload textured renderers
        ConvertTriVertsToPT_WorldXZ(roads.roadHighwayTris, textures.roadHighwayPT, 3.0f);
        ConvertTriVertsToPT_WorldXZ(roads.roadStreetTris, textures.roadStreetPT, 3.0f);
        ConvertTriVertsToPT_WorldXZ(roads.sidewalkTris, textures.sidewalkPT, 2.0f);

        textures.roadMeshHighwayTex.Upload(textures.roadHighwayPT);
        textures.roadMeshStreetTex.Upload(textures.roadStreetPT);
        textures.sidewalkMeshTex.Upload(textures.sidewalkPT);

        // build & upload highway/street line renderers
        roads.showRoads = true;
        BuildAndUploadRoadLineVerts(roads.roadNet, roads.highwayLines, roads.streetLines);

        // lot generation
        road::LotParams lotParams = MakeLotParamsFromRoadParams(gui, roads.roadParams);
        lotState.lots = lotState.lotGen.GenerateLots(roads.roadNet, lotParams);

        // debug stats
        PrintGardenPipelineStats(lotState.lots, lotParams);

        // build derived geometry (lots/sidewalk/gardens/footprints/buildings) + upload
        BuildAndUploadLotDerivedGeometry(roads, lotState, buildings, textures);

        auto t1 = Clock::now();
        const double generationMs =
            std::chrono::duration<double, std::milli>(t1 - t0).count();

        gui.SetLastGenerationMs(generationMs);
    }
}