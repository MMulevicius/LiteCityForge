#include "Road/RoadGenerator.h"
#include "Road/RoadCandidate.h"
#include "Road/RoadQuery.h"
#include "Road/RoadCandidatePolicy.h"
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

        //Spatial query helper
        RoadQuery query(net, params.queryCellSize);

        //city center/start node
        NodeId center = net.AddNode(params.cityCenter);
        query.InsertNode(center);

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
            glm::vec2 E = S + cand.dir * cand.length;
            
            //global bounds
            if (glm::length(E - params.cityCenter) > params.cityRadius)
            {
                iterations++;
                continue;
            }
            //local angle at start
            if (params.minAngleDeg > 0.0f)
            {
                int connected = 0;
                for (const auto& seg : net.Segments())
                    if (seg.a == cand.start || seg.b == cand.start)
                        connected++;

                if (connected >= 2)
                {        
                const float minAngleDeg = glm::radians(params.minAngleDeg);
                const float cosThresh = std::cos(minAngleDeg);
                
                bool ok = true;
                for (const auto& seg: net.Segments())
                {
                    if (seg.a != cand.start && seg.b != cand.start) continue;

                    glm::vec2 A = net.Nodes()[seg.a - 1].pos;
                    glm::vec2 B = net.Nodes()[seg.b - 1].pos;

                    glm::vec2 existingDir = (seg.a == cand.start) ? glm::normalize(B - A) : glm::normalize(A - B);
                    float c = glm::dot(existingDir, glm::normalize(cand.dir));
                    if(c > cosThresh) { ok = false; break; }
                }
                if (!ok) { iterations++; continue; }
                }
            }

            //local snap-to-node
            NodeId snapId = 0;
            float snapDist = 0.0f;
            bool snapped = query.FindNearestNode(E, params.snapRadius, cand.start, snapId, snapDist);
            if(snapped)
                E = net.Nodes()[snapId - 1].pos;

            //local endpoint spacing (RoadQuery)
            {
                auto nearNodes = query.QueryNearbyNodes(E, params.minNodeSpacing);
                bool tooClose = false;

                for (NodeId id: nearNodes)
                {
                     if (id == cand.start) continue;
                     if(snapped && id == snapId) continue;

                     if (glm::length(net.Nodes()[id - 1].pos - E) < params.minNodeSpacing)
                     {
                        tooClose = true;
                        break;
                     }
                }
                if (tooClose) { iterations++; continue; }
            }

            //local intersection + segment spacing
            // fetch only segments in AABB cells overlapped by candidate.
            auto nearbySeIds = query.QueryNearbySegments(S, E);

            //helper lambdas for intersection + point-to-seg distance
            auto cross2 = [](const glm::vec2& a, const glm::vec2& b) { return a.x*b.y - a.y*b.x;};
            auto segIntersect = [&](const glm::vec2& p, const glm::vec2& p2,
                                    const glm::vec2& q, const glm::vec2& q2) ->bool
            {
                glm::vec2 r = p2 - p;
                glm::vec2 s = q2 - q;
                float rxs = cross2(r, s);
                float qpxr = cross2(q - p, r);

                float eps = params.intersectionTol;

                if (std::abs(rxs) <= eps && std::abs(qpxr) <= eps)
                    return true;
                
                if (std::abs(rxs) <= eps)
                    return false;
                
                    float t = cross2(q - p, s) / rxs;
                    float u = cross2(q - p, r) / rxs;
                    return (t >= -eps && t <= 1.0f + eps && u >= -eps && u <= 1.0f + eps);
                    
            };

            auto distPointToSeg = [&](const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) -> float
            {
                glm::vec2 ab = b - a;
                float ab2 = glm::dot(ab, ab);
                if(ab2 <= 1e-8f) return glm::length(p - a);
                float t = glm::dot(p - a, ab) / ab2;
                t = std::max(0.0f, std::min(1.0f, t));
                glm::vec2 proj = a + t * ab;
                return glm::length(p- proj);
            };

            bool reject = false;
            auto nearbySegIds = query.QueryNearbySegments(S, E);
            for (SegId sid: nearbySegIds)
            {
                const auto& seg = net.Segments()[sid - 1];

                //ignore segments touching start node (same as pseudocode)
                if (seg.a == cand.start || seg.b == cand.start)
                    continue;

                glm::vec2 A = net.Nodes()[seg.a - 1].pos;
                glm::vec2 B = net.Nodes()[seg.b - 1].pos;

                //intersection reject
                if (segIntersect(S, E, A, B))
                {
                    reject = true;
                    break;
                }

                if (params.minSegmentSpacing > 0.0f)
                {
                    float d1 = distPointToSeg(S, A, B);
                    float d2 = distPointToSeg(E, A, B);
                    if (d1 < params.minSegmentSpacing || d2 < params.minSegmentSpacing)
                    {
                        reject = true;
                        break;
                    }
                }
            }

            if (reject) { iterations++; continue; }

            //accept candidate
            NodeId endNodeId;
            if(snapped)
            {
                endNodeId = snapId;
            }
            else 
            {
                endNodeId = net.AddNode(E);
                query.InsertNode(endNodeId);
            }

            //add segment
            SegId newSeg = net.AddSegment(cand.start, endNodeId, cand.type);
            query.InsertSegment(newSeg);

            //spawn next candidates using policy
            auto nextCandidates = RoadCandidatePolicy::SpawnNextCandidates(params, rng, endNodeId, cand.dir, cand.type);
            for (auto& n : nextCandidates)
            {
                const glm::vec2 endPos = net.Nodes()[endNodeId - 1].pos;
                n.priority = ComputePriority(params, n.type, endPos);
                pq.push(n);
            }

            iterations++;

            //accept
            // NodeId endNode = net.AddNode(E);
            // net.AddSegment(cand.start, endNode, cand.type);

            // //spawn next candidate continuing forward
            // Candidate next;
            // next.start = endNode;
            // next.dir = cand.dir;
            // next.type = cand.type;
            // next.length = cand.length;
            // next.priority = ComputePriority(params, next.type, E);

            // pq.push(next);

            // iterations++;

        }

        return net;
    }
}