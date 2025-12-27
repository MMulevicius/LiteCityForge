#include "Road/RoadNetwork.h"
#include "Road/LotTypes.h"
#include <algorithm>

namespace road
{

    NodeId RoadNetwork::AddNode(const glm::vec2 &p)
    {
        Node n;
        n.id = mNextNodeId++;
        n.pos = p;
        mNodes.push_back(n);
        return n.id;
    }

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

    void RoadNetwork::Clear()
    {
        mNodes.clear();
        mSegments.clear();
        mNextNodeId = 1;
        mNextSegId = 1;
    }

    //for lot creation
    static inline void AddLine(std::vector<glm::vec3>& v, const glm::vec3& a, const glm::vec3& b)
    {
        v.push_back(a);
        v.push_back(b);
    }

    static void AddPolygonsAsLines(std::vector<glm::vec3>& v, const std::vector<glm::vec2>& poly, float y)
    {
        if (poly.size() < 2) return;

        for (size_t i = 0; i < poly.size(); i++)
        {
            const glm::vec2& p0 = poly[i];
            const glm::vec2& p1 = poly[(i + 1) % poly.size()];

            AddLine(v,
                glm::vec3(p0.x, y, p0.y),
                glm::vec3(p1.x, y, p1.y));
        }
    }

    void BuildRoadLineVerts(const RoadNetwork& net, std::vector<glm::vec3>& outHighways,
                            std::vector<glm::vec3>& outStreets, float y)
    {
        outHighways.clear();
        outStreets.clear();

        outHighways.reserve(net.Segments().size() * 2);
        outStreets.reserve(net.Segments().size() * 2);

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

    void BuildLotLineVerts(const LotCollection& lots, std::vector<glm::vec3>& outLots, float y)
    {
        outLots.clear();
        outLots.reserve(lots.lots.size() * 8);

        for (const auto& lot : lots.lots)
        {
            AddPolygonsAsLines(outLots, lot.boundary, y);
        }
    }
}