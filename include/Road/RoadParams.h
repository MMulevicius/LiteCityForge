#pragma once 
#include <glm/glm.hpp>

namespace road
{
    struct RoadParams
    {
        //bounds
        float cityRadius = 40.0f;
        glm::vec2 cityCenter = {0.0f, 0.0f};

        //generation limits
        int maxIterations = 2000;
        int maxSegments = 250;

        //initial skeleton
        int initialRays = 4;
        float seedJitterDeg = 0.0f;

        //lengths
        float streetLength = 6.0f;
        float highwayLength = 12.0f;

        //weights
        float highwayPriorityWeight = 2.0f;
        float streetPriorWeight = 1.0f;

        //random
        unsigned int seed = 1337;
    };
}