#include "Road/RoadNetwork.h"
#include "Lots/LotTypes.h"
#include <algorithm>

namespace road
{
    namespace
    {
      constexpr std::size_t gVerticesPerLine{2};
      constexpr std::size_t gEstimatedPolygonVertexCount{8};
      constexpr std::size_t gMinNumberOfPoints{3};
    }

    //add a new intersection node to the road graph
    NodeId RoadNetwork::AddNode(const glm::vec2 &p)
    {
        Node n;
        n.id = mNextNodeId++;
        n.pos = p;
        mNodes.push_back(n);
        return n.id;
    }

    //add a road segment connecting two existing nodes
    SegId RoadNetwork::AddSegment(NodeId a, NodeId b, RoadType type)
    {
        Segment s;
        s.id = mNextSegId++;
        s.a = a;
        s.b = b;
        s.type = type;
        mSegments.push_back(s);
        return s.id;
    }

    //clears the road graph and resets node/segment ID counters
    void RoadNetwork::Clear()
    {
        mNodes.clear();
        mSegments.clear();
        mNextNodeId = 1;
        mNextSegId = 1;
    }

    //appends a single line segment as two vertices
    static inline void AddLine(std::vector<glm::vec3>& v, const glm::vec3& a, const glm::vec3& b)
    {
        v.push_back(a);
        v.push_back(b);
    }
    //converts a polygon outline into line vertices for rendering/debug drawing 
    static void AddPolygonsAsLines(std::vector<glm::vec3>& v, const std::vector<glm::vec2>& poly, float y)
    {

        if (poly.size() < gVerticesPerLine) return;

        for (size_t i = 0; i < poly.size(); i++)
        {
            const glm::vec2& p0 = poly[i];

            // +1 to target the real location
            const glm::vec2& p1 = poly[(i + 1) % poly.size()];

            AddLine(v,
                glm::vec3(p0.x, y, p0.y),
                glm::vec3(p1.x, y, p1.y));
        }
    }

    //builds seperate line vertex buffers for highways and streets
    void BuildRoadLineVerts(const RoadNetwork& net, std::vector<glm::vec3>& outHighways,
                            std::vector<glm::vec3>& outStreets, float y)
    {
      

        outHighways.clear();
        outStreets.clear();

        outHighways.reserve(net.Segments().size() * gVerticesPerLine);
        outStreets.reserve(net.Segments().size() * gVerticesPerLine);

        for (const auto& seg: net.Segments())
        {

            const auto& A = net.Nodes().at(seg.a - 1).pos;
            const auto& B = net.Nodes().at(seg.b - 1).pos;

            glm::vec3 a3(A.x, y, A.y);
            glm::vec3 b3(B.x, y, B.y);

            if (seg.type == RoadType::Highway)
                AddLine(outHighways, a3, b3);
            else
                AddLine(outStreets, a3, b3);
        }
    }

    //builds linne vertices for lot boundary outlines
    void BuildLotLineVerts(const LotCollection& lots, std::vector<glm::vec3>& outLots, float y)
    {
      
        outLots.clear();
        outLots.reserve(lots.lots.size() * gEstimatedPolygonVertexCount);

        for (const auto& lot : lots.lots)
        {
            AddPolygonsAsLines(outLots, lot.boundary, y);
        }
    }

    //garden building
    void BuildGardenLineVerts(const LotCollection& lots, std::vector<glm::vec3>& outGardens, float y)
    {
        outGardens.clear();
        outGardens.reserve(lots.lots.size() * gEstimatedPolygonVertexCount);

        for (const auto& lot : lots.lots)
        {
            if (!lot.hasGarden) continue;
            if (lot.garden.size() < gMinNumberOfPoints) continue;

            AddPolygonsAsLines(outGardens, lot.garden, y);
        }
    }

    //footprint building
    void BuildFootprintLineVerts(const LotCollection& lots, std::vector<glm::vec3>& outFootprints, float y)
    {
        outFootprints.clear();
        outFootprints.reserve(lots.lots.size() * gEstimatedPolygonVertexCount);

        for (const auto& lot : lots.lots)
        {
            if (!lot.hasFootPrint) continue;
            if (lot.footprint.size() < gMinNumberOfPoints) continue;
            AddPolygonsAsLines(outFootprints, lot.footprint, y);
        }
    }


    //checking unsplit intersections

    bool SegIntersect(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c,
                                const glm::vec2& d, glm::vec2& out)
    {

        constexpr float parallelEpsilon{1e-6f};
        constexpr float endpointEpsilon{1e-4f};

        glm::vec2 r = b - a;
        glm::vec2 s = d - c;
        float denom = r.x * s.y - r.y * s.x;
        if (std::fabs(denom) < parallelEpsilon)return false;




        glm::vec2 ac = c - a;
        float t = (ac.x * s.y - ac.y * s.x) / denom;
        float u = (ac.x * r.y - ac.y * r.x) / denom;

        if(t > endpointEpsilon && t < 1.0f - endpointEpsilon && u > endpointEpsilon && u < 1.0f - endpointEpsilon)
        {
            out = a + t * r;
            return true;
        }
        return false;

    }
    //counts segment intersections that have not been split into ndoes
    int CountUnsplitIntersections(const road::RoadNetwork& net)
    {
        int count = 0;
        const auto& nodes = net.Nodes();
        const auto& segs = net.Segments();

        auto P = [&](road::NodeId id){ return nodes.at(id - 1).pos;};

        for(size_t i = 0; i < segs.size(); i++)
        for(size_t j = i + 1; j < segs.size(); j++)
        {
            const auto& s1 = segs[i];
            const auto& s2 = segs[j];

            if(s1.a==s2.a || s1.a==s2.b ||s1.b==s2.a ||s1.b==s2.b) continue;

            glm::vec2 hit;
            if(SegIntersect(P(s1.a), P(s1.b), P(s2.a), P(s2.b), hit))
                count++;
        }
        return count;
    }
}
