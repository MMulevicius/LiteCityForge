#pragma once 
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>
#include "RoadTypes.h"

namespace road
{
    using LotId = std::uint32_t;

    enum class LotZone : std::uint8_t
    {
        Urban = 0,
        Suburban = 1,
        Rural = 2
    };

    struct Lot
    {
        LotId lotId{};
        SegId roadSegId{};
        RoadType roadType{RoadType::Street};

        std::vector<glm::vec2> boundary;

        float area = 0.0f;
        glm::vec2 centroid{0.0f, 0.0f};

        float urbanScore = 0.0f;
        LotZone zone = LotZone::Suburban;

        bool hasGarden = false;
        std::vector<glm::vec2> garden;

    };

    struct LotCollection
    {
        std::vector<Lot> lots;
    };


}