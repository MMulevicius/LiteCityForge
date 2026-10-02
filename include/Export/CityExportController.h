#pragma once

class Gui;
struct CityContext;
struct GroundContext;
struct RoadContext;
struct BuildingContext;

namespace export3d
{
    void HandleExportIfRequested(
        Gui &gui,
        const CityContext &city,
        const GroundContext &ground,
        const RoadContext &roads,
        const BuildingContext &buildings);
}