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
        int maxIterations = 6000;
        int maxSegments = 800;

        int maxHighwaySegments = 50;
        int maxStreetSegments = 750;

        //initial skeleton
        int initialRays = 4;
        float seedJitterDeg = 25.0f;

        //lengths
        float streetLength = 4.0f;
        float highwayLength = 12.0f;

        //weights
        float highwayPriorityWeight = 2.0f;
        float streetPriorWeight = 1.0f;

        //random
        unsigned int seed = 1337;

        //constraints
        float minAngleDeg = 15.0f;
        float minNodeSpacing = 1.5f;
        float minSegmentSpacing = 0.35f;
        float intersectionTol = 1e-4f;

        //spatial query grid
        float queryCellSize = 8.0f;
        float snapRadius = 1.0f;

        //candidate policy
        float branchProbabilityHighway = 0.15f;
        float branchProbabilityStreet = 0.55f;

        float branchTurnDeg = 60.0f;

        float streetFromHighwayChance = 0.85;

        //loop params
        float loopCloseRadius = 18.0f;
        float loopCloseChance = 0.10f;
        float loopClosePriorityBoost = 0.4f;

        //gridness
        float gridness = 1.0f;
        float gridAngleStepDeg = 90.0f;

        //snapping
        float segmentSnapRadius = 8.0f;
        float minSplitFromEnds = 3.0f;
    };
}