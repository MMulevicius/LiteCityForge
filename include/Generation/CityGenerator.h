#pragma once

#include "Generation/CityGenerationState.h"
#include "GUI/Gui.h"

namespace generation
{
    void GenerateCityNow(
        Gui &gui,
        RoadContext &roads,
        LotContext &lotState,
        BuildingContext &buildings,
        TextureContext &textures);
}