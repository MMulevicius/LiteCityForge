#pragma once

#include <glm/glm.hpp>

#include "Generation/CityGenerationState.h"
#include "GUI/Gui.h"
#include "Rendering/Shader.h"
#include "Rendering/RenderContexts.h"
#include "Core/Primitives.h"
#include "Rendering/ShadowMap.h"

namespace rendering
{
    void Draw3DMeshesIfEnabled(
        Gui &gui,
        bool is3D,
        Shader &litShader,
        const glm::mat4 &viewProjection,
        RoadContext &roads,
        BuildingContext &buildings,
        TextureContext &textures);

    void DrawDebugLinesIfEnabled(
        Gui &gui,
        Shader &lineShader,
        const glm::mat4 &viewProjection,
        RoadContext &roads,
        LotContext &lots);

    GroundContext DrawGroundAndGetExtents(
        Primitives &primitives,
        Shader &primShader,
        Shader &litShader,
        const glm::mat4 &viewProjection,
        const CityContext &city,
        TextureContext &textures);

    void RenderShadowPassIf3D(
        bool is3D,
        Gui &gui,
        ShadowMap &shadowMap,
        Shader &shadowDepthShader,
        const glm::mat4 &lightSpace,
        const FrameContext &frame,
        RoadContext &roads,
        BuildingContext &buildings,
        TextureContext &textures);
}