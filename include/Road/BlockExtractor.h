#pragma once
#include <vector>
#include "RoadNetwork.h"
#include "BlockTypes.h"

namespace road
{
    class BlockExtractor
    {
        public:
        std::vector<Block> ExtractBlocks(const RoadNetwork& net) const;
        float minBlockArea = 1.0f;

    };


}