#pragma once
#include "Road/RoadCandidate.h"
#include "Road/RoadParams.h"
#include <glm/glm.hpp>
#include <random>
#include <vector>

namespace road
{
    class RoadCandidatePolicy
    {
    private:
    static glm::vec2 ApplyGridness(const RoadParams& params, std::mt19937& rng, const glm::vec2& dir);
    static std::pair<RoadType, float> ChooseBranchTypeAndLength(
        const RoadParams& params, std::mt19937& rng, RoadType incomingType);

    static glm::vec2 TurnLeft(const glm::vec2& dir);
    static glm::vec2 TurnRight(const glm::vec2& dir);
    static glm::vec2 RotateDeg(const glm::vec2& v, float deg);
    static glm::vec2 SnapDirectionToNearestGridAngle(const glm::vec2& dir, float stepDeg);
    static float Random01(std::mt19937& rng);

    public:
        static std::vector<Candidate> SpawnNextCandidates(
            const RoadParams& params,
            std::mt19937& rng,
            NodeId acceptedEndNodeId,
            const glm::vec2& incomingDir,
            RoadType incomingType);
    };




}