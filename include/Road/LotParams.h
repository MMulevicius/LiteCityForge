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
        float lotCellSize = 10.0f;

        // how wide each lot is along the road
        float lotMinFrontage = 2.0f;
        float lotMaxFrontage = 10.0f;

        //fill frontage
        float fillFrontageFactor = 0.70f;
        int frontageBinarySteps = 8;
        int fillExtraAttempts = 1;

        float failAdvance = 1.0f;

        //clear radius
        float junctionClearRadius = 0.6f;
        
        //filter
        bool placeLotsOnStreets = true;
        bool placeLotsOnHighways = true;

        //random see
        std::uint32_t seed = 1337u;

        //urbanization
        float cityRadius = 40.0f;
        float globalUrbanization = 0.5f;
        float globalBiasStrength = 0.35f;

        float urbanThreshold = 0.70f;
        float suburbanThreshold = 0.40f;

        //garden rules
        float minLotAreaForGarden = 6.0f;
        float gardenBackRatio = 0.45f;
        float minGardenDepth = 0.8f;

        //building foorprint
        float buildingSetbackFront = 0.25f;
        float buildingSetbackSide = 0.20f;
        float buildingSetbackBack = 0.20f;

        //footprint variability
        float buildingSetBackJitter = 0.10f;
        float buildingCoverageMin = 0.25f;
        float buildingCoverageMax = 0.55f;

        //floor ranges per zone
        int floorsUrbanMin = 2;
        int floorsUrbanMax = 8;
        int floorsSuburbanMin = 1;
        int floorsSuburbanMax = 3;
        int floorsRuralMin = 1;
        int floorsRuralMax = 2;

        //continues heights based on urbanization level
        bool useContinuousHeight = true;

        //rural
        int floorsEdgeMin = 1;
        int floorsEdgeMax = 2;
        //sub-urban
        int floorsMidMin = 2;
        int floorsMidMax = 6;
        //urban
        int floorsCenterMin = 12;
        int floorsCenterMax = 20;

        float floorsRandomness = 0.35f;
        float minHeightScaleAtZeroUrbanization = 0.35f;



    };

}