#pragma once
#include "Road/RoadCandidate.h"
#include "Road/RoadParams.h"
#include "Road/RoadTypes.h"
#include <glm/glm.hpp>
#include <random>
#include <vector>

namespace road
{
    //defines how new road candidates are generated
    class RoadCandidatePolicy
    {
    private:
    //grid bias to the direction vector
    static glm::vec2 ApplyGridness(const RoadParams& params, std::mt19937& rng, const glm::vec2& dir);
    //rotates a direction vector by a specified angle in degrees
    static glm::vec2 RotateDeg(const glm::vec2& v, float deg);
    //snaps a direction to the nearest grid angle
    static glm::vec2 SnapDirectionToNearestGridAngle(const glm::vec2& dir, float stepDeg);
    //generates a random float
    static float Random01(std::mt19937& rng);

    public:
        //generates the next set of road candidates from an accepted node
        static std::vector<Candidate> SpawnNextCandidates(
            const RoadParams& params,
            std::mt19937& rng,
            NodeId acceptedEndNodeId,
            const glm::vec2& incomingDir,
            RoadType incomingType,
            GenerationPhase phase);
    };
}