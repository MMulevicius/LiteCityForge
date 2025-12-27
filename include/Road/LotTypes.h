#pragma once 
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>
#include "RoadTypes.h"

namespace road
{
    using LotId = std::uint32_t;

    struct Lot
    {
        LotId lotId{};
        SegId roadSegId{};
        RoadType roadType{RoadType::Street};

        std::vector<glm::vec2> boundary;

    };

    struct LotCollection
    {
        std::vector<Lot> lots;
    };


}