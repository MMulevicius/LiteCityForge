#pragma once 
#include "RoadNetwork.h"
#include "RoadParams.h"

namespace road
{
    //core logic of road generation
    class RoadGenerator
    {
    private:
        float ComputePriority(const RoadParams &params, RoadType type, const glm::vec2 &pos) const; 
        
    public:
        RoadNetwork Generate(const RoadParams &params);
    };
}