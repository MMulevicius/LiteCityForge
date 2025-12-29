#pragma once
#include <vector>
#include <glm/glm.hpp>

namespace road
{
    class RoadNetwork;
    class RoadParams;

    //builds a simple global sidewalk layers with two lines (left and right) for every road segment
    void BuildSidewalkLineVerts(const RoadNetwork& net, const RoadParams& params,
                                std::vector<glm::vec3>& outSidewalks, float y = 0.06f);
                            
}