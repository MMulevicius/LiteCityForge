#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

#include "Rendering/BuildingRenderer.h"

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