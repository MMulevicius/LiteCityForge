#pragma once 
#include <glm/glm.hpp>
#include "RoadTypes.h"

namespace road 
{
    //candidate for possible street segment
    struct Candidate
    {
        NodeId start{};
        glm::vec2 dir {1.0f, 0.0f}; 
        float length {6.0f};
        RoadType type = RoadType::Street;
        float priority {0.0f};
    };
}