#include "Road/RoadGenerator.h"
#include "Road/RoadCandidate.h"
#include <queue>
#include <random>
#include <cmath>


namespace road 
{
    //compares two segments and decides on the priority (used in priority_queue)
    struct CandGreater
    {
        bool operator()(const Candidate &a, const Candidate &b) const
        {
            return a.priority < b.priority;
        }
    };
    //takes an angle from radians and returns a 2D vector
    static glm::vec2 UnitFromAngle(float rad)
    {
        return glm::vec2(std::cos(rad), std::sin(rad));
    }

    //converts degrees to radians
    static float DegToRad(float deg) { return deg * 3.1415926535f / 180.0f; }

    //creates priority based on density and distance from centre
    float RoadGenerator::ComputePriority(const RoadParams &params, RoadType type, const glm::vec2 &pos) const 
    {
        float dist = glm::length(pos - params.cityCenter);
        float density = 1.0f - (dist / params.cityRadius);

        //clamps density
        if (density < 0.0f) density = 0.0f;
        if (density > 1.0f) density = 1.0f;
        
        //decides if street or highway priority
        float typeWeight = (type == RoadType::Highway) ? params.highwayPriorityWeight : params.streetPriorWeight;
        return typeWeight * density;
    }

    RoadNetwork RoadGenerator::Generate(const RoadParams &params)
    {
        RoadNetwork net;
        //random number generator (Mersenne Twister)
        std::mt19937 rng(params.seed);
        std::uniform_real_distribution<float> jitter(-params.seedJitterDeg, params.seedJitterDeg);


        //Priority queue of candidates
        std::priority_queue<Candidate, std::vector<Candidate>, CandGreater> pq;

        //city center/start node
        NodeId center = net.AddNode(params.cityCenter);

        //creates initial directional lines from the centre based on number of rays 
        for (int i = 0; i < params.initialRays; i++)
        {
            float baseDeg = (360.0f * (float)i) / (float)params.initialRays;
            float ang = DegToRad(baseDeg + jitter(rng));

            Candidate c;
            c.start = center;
            c.dir = glm::normalize(UnitFromAngle(ang));
            c.type = RoadType::Highway;
            c.length = params.highwayLength;
            c.priority = ComputePriority(params, c.type, params.cityCenter);
            
            //pushed to priority queue
            pq.push(c);

        }

        int iterations = 0;
        //grows while pq is not empty and both iterations and segments aren't exceeding limits
        while (!pq.empty() &&
                iterations < params.maxIterations &&
                (int)net.Segments().size() < params.maxSegments)
        {
            //pulls the highest priority candidate
            Candidate cand = pq.top();
            pq.pop();

            //computes segment start and end positions
            //S - start position of the segment
            //E - end position of the segment
            const glm::vec2 S = net.Nodes().at(cand.start - 1).pos;
            const glm::vec2 E = S + cand.dir * cand.length;

            //accept
            NodeId endNode = net.AddNode(E);
            net.AddSegment(cand.start, endNode, cand.type);

            //spawn next candidate continuing forward
            Candidate next;
            next.start = endNode;
            next.dir = cand.dir;
            next.type = cand.type;
            next.length = cand.length;
            next.priority = ComputePriority(params, next.type, E);

            pq.push(next);

            iterations++;

        }

        return net;
    }
}