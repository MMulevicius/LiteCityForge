#pragma once
#include "RoadNetwork.h"
#include "LotTypes.h"
#include "LotParams.h"

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