#pragma once 
#include "RoadNetwork.h"
#include "RoadParams.h"

namespace road
{
    //core logic of road generation
    class RoadGenerator
    { 
        
    public:
        float ComputePriority(const RoadParams& params, RoadType type, const glm::vec2& pos) const;
        RoadNetwork Generate(const RoadParams& params);
    };
}