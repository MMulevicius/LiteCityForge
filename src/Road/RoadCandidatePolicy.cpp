#include "Road/RoadCandidatePolicy.h"
#include <cmath>

namespace road 
{
    float RoadCandidatePolicy::Random01(std::mt19937& rng)
    {
        static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(rng);
    }

    glm::vec2 RoadCandidatePolicy::TurnLeft(const glm::vec2& dir)
    {
        // 90 degrees left: (x,y) -> (-y, x)
        return glm::vec2(-dir.y, dir.x);
    }

    glm::vec2 RoadCandidatePolicy::TurnRight(const glm::vec2& dir)
    {
        // 90 degrees right: (x,y) -> (y, -x)
        return glm::vec2(dir.y, -dir.x);
    }
    glm::vec2 RoadCandidatePolicy::RotateDeg(const glm::vec2& v, float deg)
    {
        float r = glm::radians(deg);
        float c = std::cos(r), s = std::sin(r);
        return glm::vec2(v.x * c - v.y * s, v.x * s + v.y * c);
    }

    static float AngleOf(const glm::vec2& d)
    {
        return std::atan2(d.y, d.x);
    }

    static glm::vec2 DirFromAngle(float a)
    {
        return glm::vec2(std::cos(a), std::sin(a));
    }

    glm::vec2 RoadCandidatePolicy::SnapDirectionToNearestGridAngle(const glm::vec2& dir, float stepDeg)
    {
        glm::vec2 nd = glm::normalize(dir);
        float stepRad= glm::radians(stepDeg);
        float a = AngleOf(nd);
        float snapped = std::round(a / stepRad) * stepRad;
        return glm::normalize(DirFromAngle(snapped));
    }

    glm::vec2 RoadCandidatePolicy::ApplyGridness(const RoadParams& params, std::mt19937& rng, const glm::vec2& dir)
    {
        glm::vec2 snapped = SnapDirectionToNearestGridAngle(dir, params.gridAngleStepDeg);
        if(Random01(rng) < params.gridness)
            return snapped;

        return glm::normalize(dir);
    }

    std::pair<RoadType, float> RoadCandidatePolicy::ChooseBranchTypeAndLength(
        const RoadParams& params, std::mt19937& rng, RoadType incomingType)
    {
        //highways may spawn street
        if (incomingType == RoadType::Highway && Random01(rng) < params.streetFromHighwayChance)
            return { RoadType::Street, params.streetLength };

        return { RoadType::Street, params.streetLength };
    }

    std::vector<Candidate> RoadCandidatePolicy::SpawnNextCandidates(
        const RoadParams& params,
        std::mt19937& rng,
        NodeId acceptedEndNodeId,
        const glm::vec2& incomingDir,
        RoadType incomingType)
    {
        std::vector<Candidate> out;

        float branchProb = (incomingType == RoadType::Highway)
            ? params.branchProbabilityHighway
            : params.branchProbabilityStreet;

        //always spawn forward
        {
            glm::vec2 forwardDir = ApplyGridness(params, rng, incomingDir);

            Candidate c;
            c.start = acceptedEndNodeId;
            c.dir = forwardDir;
            c.type = incomingType;
            c.length = (incomingType == RoadType::Highway) ? params.highwayLength : params.streetLength;
            out.push_back(c);
        }

        //probabilistic left/right branches
        if (Random01(rng) < branchProb)
        {
            glm::vec2 leftDir = ApplyGridness(params, rng, RotateDeg(incomingDir, +45.0f));
            glm::vec2 rightDir = ApplyGridness(params, rng, RotateDeg(incomingDir, -45.0f));

            auto [t1, l1] = ChooseBranchTypeAndLength(params, rng, incomingType);
            auto [t2, l2] = ChooseBranchTypeAndLength(params, rng, incomingType);

            Candidate left;
            left.start = acceptedEndNodeId;
            left.dir = leftDir;
            left.type = t1;
            left.length = l1;
            out.push_back(left);

            Candidate right;
            right.start = acceptedEndNodeId;
            right.dir = rightDir;
            right.type = t2;
            right.length = l2;
            out.push_back(right);
        }

        return out;
    }


}