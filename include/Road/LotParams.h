#pragma once
#include <cstdint>

namespace road
{
    struct LotParams
    {
        //Road widths
        float streetHalfWidth = 0.35f;
        float highwayHalfWidth = 0.60f;

        // gap between road edge and lot start
        float lotSetBackFromRoad = 0.25f;

        // how deep the lot goes away from the road
        float lotDepth = 4.0f;

        // how wide each lot is along the road
        float lotMinFrontage = 2.0f;
        float lotMaxFrontage = 6.0f;
        
        // filter
        bool placeLotsOnStreets = true;
        bool placeLotsOnHighways = false;

        // random see
        std::uint32_t seed = 1337u;



    };

}