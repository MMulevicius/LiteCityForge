#pragma once
#include "Road/RoadNetwork.h"
#include "Lots/LotTypes.h"
#include "Lots/LotParams.h"

namespace road
{
    class LotSubdivision
    {
        public:

            //main entry point
            LotCollection GenerateLots(const RoadNetwork& net, const LotParams& params);

        private:
            static bool IsLotRoadType(RoadType t, const LotParams& params);

    };
}