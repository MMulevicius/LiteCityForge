#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

#include "Lots/LotSubdivision.h"
#include "Lots/LotTypes.h"
#include "Rendering/BuildingRenderer.h"
#include "Rendering/LineRenderer.h"
#include "Road/RoadGenerator.h"
#include "Road/RoadNetwork.h"
#include "Road/RoadParams.h"

struct BuildingContext
{
    // building mesh variable
    road::BuildingRenderer buildingMesh;
    road::BuildingRenderer roofMesh;
    road::BuildingRenderer windowMesh;

    // building detail tris
    std::vector<glm::vec3> buildingTriVerts;
    std::vector<glm::vec3> buildingRoofTriVerts;
    std::vector<glm::vec3> buildingWindowTriVerts;

    // building detail quads
    std::vector<glm::vec3> buildingQuadVerts;
    std::vector<glm::vec3> buildingRoofQuadVerts;
    std::vector<glm::vec3> buildingWindowQuadVerts;
};

struct RoadContext
{
    // road variables
    bool showRoads = false;
    road::RoadGenerator roadGen;
    road::RoadParams roadParams;
    road::RoadNetwork roadNet;
    road::LineRenderer highwayLines;
    road::LineRenderer streetLines;
    std::vector<glm::vec3> roadLineVerts;

    // roads and sidewalks mesh variables
    road::BuildingRenderer roadMeshHighway;
    road::BuildingRenderer roadMeshStreet;
    road::BuildingRenderer sidewalkMesh;

    std::vector<glm::vec3> roadHighwayTris;
    std::vector<glm::vec3> roadStreetTris;
    std::vector<glm::vec3> sidewalkTris;

    // sidewalk variables
    road::LineRenderer sidewalkLines;
    std::vector<glm::vec3> sidewalkLineVerts;
};

struct LotContext
{

    // lot variables
    road::LotSubdivision lotGen;
    road::LotCollection lots;
    road::LineRenderer lotLines;
    std::vector<glm::vec3> lotLineVerts;

    // garden variables
    road::LineRenderer gardenLines;
    std::vector<glm::vec3> gardenLineVerts;

    // building footprint variables
    road::LineRenderer footprintLines;
    std::vector<glm::vec3> footprintLineVerts;
};

struct TextureContext
{

    // building window texture variables
    road::BuildingTexturedRenderer windowMeshTex;

    // roof texture variables
    road::BuildingTexturedRenderer roadMeshHighwayTex;
    road::BuildingTexturedRenderer roadMeshStreetTex;
    road::BuildingTexturedRenderer sidewalkMeshTex;
    road::BuildingTexturedRenderer groundMeshTex;

    std::vector<road::BuildingVertexPT> roofUrbanPT;
    std::vector<road::BuildingVertexPT> roofSuburbanPT;
    std::vector<road::BuildingVertexPT> roofRuralPT;
    std::vector<road::BuildingVertexPT> roadHighwayPT;
    std::vector<road::BuildingVertexPT> roadStreetPT;
    std::vector<road::BuildingVertexPT> sidewalkPT;
    std::vector<road::BuildingVertexPT> groundPT;

    // roof meshes by type
    road::BuildingTexturedRenderer roofMeshUrban;
    road::BuildingTexturedRenderer roofMeshSuburban;
    road::BuildingTexturedRenderer roofMeshRural;

    // building texture variables
    road::BuildingTexturedRenderer buildingMeshUrban;
    road::BuildingTexturedRenderer buildingMeshSuburban;
    road::BuildingTexturedRenderer buildingMeshRural;

    std::vector<road::BuildingVertexPT> buildingUrbanPT;
    std::vector<road::BuildingVertexPT> buildingSuburbanPT;
    std::vector<road::BuildingVertexPT> buildingRuralPT;

    // window texture
    GLuint windowTex = 0;
    GLuint roadTex = 0;
    GLuint sidewalkTex = 0;
    GLuint groundTex = 0;

    bool useWindowTex = false;
    bool useRoadTex = false;
    bool useSidewalkTex = false;
    bool useGroundTex = false;

    GLuint urbanTex = 0, suburbanTex = 0, ruralTex = 0;
    bool useUrban = false, useSuburban = false, useRural = false;

    GLuint roofUrbanTex = 0, roofSuburbanTex = 0, roofRuralTex = 0;
    bool useRoofUrban = false, useRoofSuburban = false, useRoofRural = false;
};
