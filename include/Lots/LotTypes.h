#pragma once 
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>
#include "Road/RoadTypes.h"

namespace road
{
    //id 
    using LotId = std::uint32_t;

    //lot types
    enum class LotZone : std::uint8_t
    {
        Urban = 0,
        Suburban = 1,
        Rural = 2
    };

    //lot details
    struct Lot
    {
        // Ids and type
        LotId lotId{};
        SegId roadSegId{};
        RoadType roadType{RoadType::Street};

        //polygon boundary of the lot
        std::vector<glm::vec2> boundary;

        //precomputed lot area and geometric centre of the lot
        float area = 0.0f;
        glm::vec2 centroid{0.0f, 0.0f};

        //score for zoning classification
        float urbanScore = 0.0f;
        LotZone zone = LotZone::Suburban;

        //garden area polygon if present
        bool hasGarden = false;
        std::vector<glm::vec2> garden;

        //building footprint polygon
        std::vector<glm::vec2> footprint;
        bool hasFootPrint = false;

        //lot coverage ratio and generated building height
        float coverage = 0.0f;
        int floors = 1;

    };

    //container for generated lots 
    struct LotCollection
    {
        std::vector<Lot> lots;
    };


}