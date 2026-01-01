#pragma once
#include <vector>
#include <glm/glm.hpp>

namespace road
{
    class RoadNetwork;
    struct RoadParams;

    // builds triangle verts for extruded road slabs
    void BuildRoadSurfaceTriVerts(const RoadNetwork& net,
                                  const RoadParams& params,
                                  std::vector<glm::vec3>& outHighwayTris,
                                  std::vector<glm::vec3>& outStreetTris,
                                  float roadBaseY = 0.02f,
                                  float roadHeight = 0.02f);

    // builds triangle verts for sidewalks
    void BuildSidewalkSurfaceTriVerts(const RoadNetwork& net,
                                      const RoadParams& params,
                                      std::vector<glm::vec3>& outSidewalkTris,
                                      float sidewalkBaseY = 0.04f,
                                      float sidewalkHeight = 0.08f,
                                      float junctionTrim = 0.40f,
                                      bool streetsOnly = true);
}
