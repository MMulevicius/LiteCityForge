#pragma once

#include "GUI/Gui.h"
#include <glad/glad.h>
#include "Textures/TextureContext.h"

namespace textures
{
    void HandleWindowTextureRequests(
        Gui &gui,
        TextureContext &textureState);

    void HandleSceneTextureRequests(
        Gui &gui,
        TextureContext &textureState);

    void HandleBuildingTextureRequests(
        Gui &gui,
        TextureContext &textureState);

    void HandleRoofTextureRequests(
        Gui &gui,
        TextureContext &textureState);
}