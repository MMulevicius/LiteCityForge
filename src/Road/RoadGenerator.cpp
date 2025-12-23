#include "Road/RoadGenerator.h"
#include "Road/RoadCandidate.h"
#include "Road/RoadQuery.h"
#include "Road/RoadCandidatePolicy.h"
#include <queue>
#include <random>
#include <cmath>


namespace road 
{
    enum class AcceptMode
    {
        Reject,
        Normal,
        SplitSeg
    };

    struct AcceptResult
    {
        AcceptMode mode = AcceptMode::Reject;
        glm::vec2 S{};
        glm::vec2 E{};
        bool snappedToNode = false;
        NodeId snapNodeId = 0;

        SegId hitSegId = 0;
        glm::vec2 hitPoint{};
    };
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

    //geometry helper 
    static float Cross2(const glm::vec2& a, const glm::vec2& b) { return a.x * b.y - a.y * b.x;}

    static float DistPointtoSeg (const glm::vec2& p, const glm::vec2& a, const glm::vec2& b)
    {
        glm::vec2 ab = b - a;
        float ab2 = glm::dot(ab, ab);
        if (ab2 <= 1e-8f) return glm::length(p - a);

        float t = glm::dot(p - a, ab) /ab2;
        t = std::max(0.0f, std::min(1.0f, t));
        glm::vec2 proj = a + t * ab;
        return glm::length(p - proj);

    }
    static void SeedStreetsFromHighways(const RoadParams& params, std::mt19937& rng, const RoadNetwork& net,
                                        std::priority_queue<Candidate, std::vector<Candidate>, CandGreater>& pq,
                                        const RoadGenerator& gen)
    {
        std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
        for (const auto& seg : net.Segments())
        {
            if(seg.type != RoadType::Highway)
                continue;

            const glm::vec2 A = net.Nodes()[seg.a - 1].pos;
            const glm::vec2 B = net.Nodes()[seg.b - 1].pos;

            glm::vec2 dir = glm::normalize(B - A);

            //street limiter
            const float seedChance = params.streetFromHighwayChance;
            if (dist01(rng) > seedChance)
                continue;

            //seed left and/or right streets
            glm::vec2 leftDir = glm::normalize(glm::vec2(-dir.y, dir.x));
            glm::vec2 rightDir = glm::normalize(glm::vec2(dir.y, -dir.x));

            auto pushStreet = [&](NodeId start, const glm::vec2& d)
            {
                Candidate c;
                c.start = start;
                c.dir = d;
                c.type = RoadType::Street;
                c.length = params.streetLength;
                c.priority = gen.ComputePriority(params, c.type, net.Nodes()[start - 1].pos);
                pq.push(c);
            };
            pushStreet(seg.a, leftDir);
            pushStreet(seg.a, rightDir);
            pushStreet(seg.b, leftDir);
            pushStreet(seg.b, rightDir);
        }
    }
    static bool SegIntersectLoose(const glm::vec2& p, const glm::vec2& p2, const glm::vec2& q, const glm::vec2& q2, float eps)
    {
        glm::vec2 r = p2 - p;
        glm::vec2 s = q2 - q;
        float rxs = Cross2(r, s);
        float qpxr = Cross2(q - p, r);

        //collinear treated as intersect
        if(std::abs(rxs) <= eps && std::abs(qpxr) <= eps)
            return false; 

        //parallel non-collinear
        if(std::abs(rxs) <= eps)
            return false;

        float t = Cross2(q - p, s) / rxs;
        float u = Cross2(q - p, r) / rxs;
        return (t >= -eps && t <= 1.0f + eps && u >= -eps && u <= 1.0f + eps);
    }

    static void SeedInitialRays(const RoadParams& params, std::mt19937& rng, NodeId center,
                                 std::priority_queue<Candidate, std::vector<Candidate>, CandGreater>& pq,
                                const RoadGenerator& gen)
    {

        std::uniform_real_distribution<float> jitter(-params.seedJitterDeg, params.seedJitterDeg);

        for (int i = 0; i < params.initialRays; i++)
        {
            float baseDeg = (360.0f * (float)i) / (float)params.initialRays;
            float ang = DegToRad(baseDeg + jitter(rng));

            Candidate c;
            c.start = center;
            c.dir = glm::normalize(UnitFromAngle(ang));
            c.type = RoadType::Highway;
            c.length = params.highwayLength;
            c.priority = gen.ComputePriority(params, c.type, params.cityCenter);

            pq.push(c);
        }

    }

    static bool PassesGlobalBounds(const RoadParams& params, const glm::vec2& endPos)
    {
        return glm::length(endPos - params.cityCenter) <= params.cityRadius;
    }

    static bool PassesMinAngleAtStart(const RoadParams& params, const RoadNetwork& net, const Candidate& cand)
    {
        if (params.minAngleDeg <= 0.0f)
             return true;

        int connected = 0;
        for (const auto& seg : net.Segments())
             if (seg.a == cand.start || seg.b == cand. start)
                connected++;
            
        if (connected < 2)
            return true;
        
        const float minAngleRad = glm::radians(params.minAngleDeg);
        const float cosTresh = std::cos(minAngleRad);

        for (const auto& seg : net.Segments())
        {
            if (seg.a != cand.start && seg.b != cand.start) continue;

            glm::vec2 A = net.Nodes()[seg.a - 1].pos;
            glm::vec2 B = net.Nodes()[seg.b - 1].pos;

            glm::vec2 existingDir = (seg.a == cand.start)
                ? glm::normalize(B - A)
                : glm::normalize(A - B);
                
            float c = glm::dot(existingDir, glm::normalize(cand.dir));
            if(c > cosTresh)
                return false;
        }       
        return true;
    }

    static bool TrySnapEndpoint(const RoadParams& params, RoadQuery& query, const RoadNetwork& net,
                                const Candidate& cand, glm::vec2& inOutEndPos, bool& outSnapped,
                                NodeId& outSnapId)
    {
        outSnapped = false;
        outSnapId = 0;

        float snapDist = 0.0f;
        NodeId snapId = 0;
        bool snapped = query.FindNearestNode(inOutEndPos, params.snapRadius, cand.start, snapId, snapDist);

        if (snapped)
        {
            inOutEndPos = net.Nodes()[snapId - 1].pos;
            outSnapped = true;
            outSnapId = snapId;
        }
        return true;
    }

    static bool PassesMindNodeSpacing(const RoadParams& params, RoadQuery& query, const RoadNetwork& net,
                                        const Candidate& cand, const glm::vec2& endPos, bool snapped, NodeId snapId)
    {
        auto nearNodes = query.QueryNearbyNodes(endPos, params.minNodeSpacing);

        for (NodeId id : nearNodes)
        {
            if (id == cand.start) continue;
            if (snapped && id == snapId) continue;

            if (glm::length(net.Nodes()[id - 1].pos - endPos) < params.minNodeSpacing)
                return false;
        }
        return true;
    }

    static bool PassesIntersectionAndSegSpacing(const RoadParams& params, RoadQuery& query, const RoadNetwork& net,
                                                const Candidate& cand, const glm::vec2& S, const glm::vec2& E)
    {
        auto nearbySegIds = query.QueryNearbySegments(S, E);

        for (SegId sid : nearbySegIds)
        {
            const auto& seg = net.Segments()[sid - 1];

            if(seg.a == cand.start || seg.b == cand.start)
                continue;

            glm::vec2 A = net.Nodes()[seg.a - 1].pos;
            glm::vec2 B = net.Nodes()[seg.b - 1].pos;

            //intersection reject
            if(SegIntersectLoose(S, E, A, B, params.intersectionTol))
            {
                if (cand.type == RoadType::Street)
                    continue;
                return false;
            }
            
            //segment spacing reject
            if (params.minSegmentSpacing > 0.0f)
            {
                float d1 = DistPointtoSeg(S, A, B);
                float d2 = DistPointtoSeg(E, A, B);
                if(d1 < params.minSegmentSpacing || d2 < params.minSegmentSpacing)
                    return false;
            }
        }
        return true;
    }

    static bool EvaluateCandidate(const RoadParams& params,std::mt19937& rng, RoadQuery& query, const RoadNetwork& net,
                                    const Candidate& cand, glm::vec2& outS, glm::vec2& outE, bool& outSnapped,
                                    NodeId& outSnapId)
    {
        outS = net.Nodes().at(cand.start - 1).pos;
        outE = outS + cand.dir * cand.length;

        if(!PassesGlobalBounds(params, outE))
            return false;

        if(!PassesMinAngleAtStart(params, net, cand))
            return false;

        //TrySnapEndpoint(params, query, net, cand, outE, outSnapped, outSnapId);
        {
            bool loopSnapped = false;
            NodeId loopId = 0;
            float loopDist = 0.0f;


            static std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
            float r = dist01(rng);

            if (r < params.loopCloseChance)
            {
                if (query.FindNearestNode(outE, params.loopCloseRadius, cand.start, loopId, loopDist))
                {
                    outE = net.Nodes()[loopId - 1].pos;
                    outSnapped = true;
                    outSnapId = loopId;
                    loopSnapped = true;
                }
            }

            if (!loopSnapped)
            {
                TrySnapEndpoint(params, query, net, cand, outE, outSnapped, outSnapId);
            }
        }

        if(!PassesMindNodeSpacing(params, query, net, cand, outE, outSnapped, outSnapId))
        {
            if(outSnapped)            
                return true;
            return false;
        }
        if(!PassesIntersectionAndSegSpacing(params, query, net, cand, outS, outE))
            return false;

        return true;
    }

    static void AcceptCandidateAndEnequeuNext(const RoadParams& params, std::mt19937& rng, RoadNetwork& net,
                                                RoadQuery& query, std::priority_queue<Candidate, std::vector<Candidate>, CandGreater>& pq,
                                                const RoadGenerator& gen, const Candidate& cand, const glm::vec2& endPos,
                                                bool snapped, NodeId snapId, GenerationPhase phase)
    {
        NodeId endNodeId = 0;

        if(snapped)
        {
            endNodeId = snapId;
        }
        else
        {
            endNodeId = net.AddNode(endPos);
            query.InsertNode(endNodeId);
        }

        SegId newSeg = net.AddSegment(cand.start, endNodeId, cand.type);
        query.InsertSegment(newSeg);

        auto nextCandidates = RoadCandidatePolicy::SpawnNextCandidates(params, rng, endNodeId, cand.dir, cand.type, phase);
        const glm::vec2 endNodesPos = net.Nodes()[endNodeId - 1].pos;

        for (auto& n : nextCandidates)
        {
            n.priority = gen.ComputePriority(params, n.type, endNodesPos);

            if(phase == GenerationPhase::Streets)
                n.priority += params.loopClosePriorityBoost;
            pq.push(n);
        }
    }

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

        std::mt19937 rng(params.seed);

        std::priority_queue<Candidate, std::vector<Candidate>, CandGreater> pq;

        RoadQuery query(net, params.queryCellSize);

        NodeId center = net.AddNode(params.cityCenter);
        query.InsertNode(center);

        SeedInitialRays(params, rng, center, pq, *this);

        int iterations = 0;

        GenerationPhase phase = GenerationPhase::Highways;

        while (!pq.empty() &&
                iterations < params.maxIterations &&
                (int)net.Segments().size() < params.maxSegments)
        {
            Candidate cand = pq.top();
            pq.pop();

            glm::vec2 S, E;
            bool snapped = false;
            NodeId snapId = 0;
            
            if(!EvaluateCandidate(params, rng, query, net, cand, S, E, snapped, snapId))
            {
                iterations++;
                continue;
            }

            AcceptCandidateAndEnequeuNext(params, rng, net, query, pq, *this, cand, E, snapped, snapId, phase);

            iterations++;

        }

            phase = GenerationPhase::Streets;
            iterations = 0;

            SeedStreetsFromHighways(params, rng, net, pq, *this);

            size_t segsAtStartOfStreets = net.Segments().size();

            while (!pq.empty() &&
                    iterations < params.maxIterations &&
                    (net.Segments().size() - segsAtStartOfStreets) < (size_t)params.maxStreetSegments)
            {
                Candidate cand = pq.top();
                pq.pop();

                glm::vec2 S, E;
                bool snapped = false;
                NodeId snapId = 0;
                
                if(!EvaluateCandidate(params, rng, query, net, cand, S, E, snapped, snapId))
                {
                    iterations++;
                    continue;
                }

                AcceptCandidateAndEnequeuNext(params, rng, net, query, pq, *this, cand, E, snapped, snapId, phase);
                iterations++;
            }

        return net;
    }
}