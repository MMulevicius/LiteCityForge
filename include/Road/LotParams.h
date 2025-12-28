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

        //extra spacing between lots
        float lotPadding = 0.08f;

        //depth
        float minDepthScale = 0.45f;
        int depthBinarySteps = 10;

        //depth for highways
        float highwayLotDepth = 2.5f;
        float highwayLotSetback = 0.5f;


        //spatial hashing
        float lotCellSize = 6.0f;

        // how wide each lot is along the road
        float lotMinFrontage = 2.0f;
        float lotMaxFrontage = 6.0f;

        //fill frontage
        float fillFrontageFactor = 0.70f;
        int frontageBinarySteps = 8;
        int fillExtraAttempts = 1;

        float failAdvance = 1.0f;

        //clear radius
        float junctionClearRadius = 0.6f;
        
        // filter
        bool placeLotsOnStreets = true;
        bool placeLotsOnHighways = true;

        // random see
        std::uint32_t seed = 1337u;



    };

}