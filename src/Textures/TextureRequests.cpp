#include "Textures/TextureRequests.h"
#include "Textures/TextureUtils.h"
#include <iostream>

namespace textures
{
    // handles GUI requests for applying or clearing the window texture
    void HandleWindowTextureRequests(
        Gui &gui,
        TextureContext &textureState)
    {
        std::string path;

        if (gui.ConsumeWindowTextureApply(path))
        {
            textures::SafeDeleteTexture(textureState.windowTex);
            textureState.windowTex = textures::LoadTexture2D(path);
            textureState.useWindowTex = (textureState.windowTex != 0);
        }

        if (gui.ConsumeWindowTextureClear())
        {
            textures::SafeDeleteTexture(textureState.windowTex);
            textureState.useWindowTex = false;
        }
    }
    // handles GUI requests for road, sidewlak and ground textures
    void HandleSceneTextureRequests(
        Gui &gui,
        TextureContext &textureState)
    {
        std::string path;

        if (gui.ConsumeRoadTextureApply(path))
        {
            textures::SafeDeleteTexture(textureState.roadTex);
            textureState.roadTex = textures::LoadTexture2D(path);
            textureState.useRoadTex = (textureState.roadTex != 0);
        }
        if (gui.ConsumeRoadTextureClear())
        {
            textures::SafeDeleteTexture(textureState.roadTex);
            textureState.useRoadTex = false;
        }

        if (gui.ConsumeSidewalkTextureApply(path))
        {
            textures::SafeDeleteTexture(textureState.sidewalkTex);
            textureState.sidewalkTex = textures::LoadTexture2D(path);
            textureState.useSidewalkTex = (textureState.sidewalkTex != 0);
        }
        if (gui.ConsumeSidewalkTextureClear())
        {
            textures::SafeDeleteTexture(textureState.sidewalkTex);
            textureState.useSidewalkTex = false;
        }

        if (gui.ConsumeGroundTextureApply(path))
        {
            textures::SafeDeleteTexture(textureState.groundTex);
            textureState.groundTex = textures::LoadTexture2D(path);
            textureState.useGroundTex = (textureState.groundTex != 0);
        }
        if (gui.ConsumeGroundTextureClear())
        {
            textures::SafeDeleteTexture(textureState.groundTex);
            textureState.useGroundTex = false;
        }
    }

    // handles GUI requests for urban, suburban and rural building textures
    void HandleBuildingTextureRequests(
        Gui &gui,
        TextureContext &textureState)
    {
        std::string path;

        // urban
        if (gui.ConsumeBuildingTextureApply(road::LotZone::Urban, path))
        {
            std::cout << "Loaded urbanTex=" << textureState.urbanTex << "\n";
            textures::SafeDeleteTexture(textureState.urbanTex);
            textureState.urbanTex = textures::LoadTexture2D(path);
            textureState.useUrban = (textureState.urbanTex != 0);
        }
        if (gui.ConsumeBuildingTextureClear(road::LotZone::Urban))
        {
            textures::SafeDeleteTexture(textureState.urbanTex);
            textureState.useUrban = false;
        }

        // suburban
        if (gui.ConsumeBuildingTextureApply(road::LotZone::Suburban, path))
        {
            textures::SafeDeleteTexture(textureState.suburbanTex);
            textureState.suburbanTex = textures::LoadTexture2D(path);
            textureState.useSuburban = (textureState.suburbanTex != 0);
        }
        if (gui.ConsumeBuildingTextureClear(road::LotZone::Suburban))
        {
            textures::SafeDeleteTexture(textureState.suburbanTex);
            textureState.useSuburban = false;
        }

        // rural
        if (gui.ConsumeBuildingTextureApply(road::LotZone::Rural, path))
        {
            textures::SafeDeleteTexture(textureState.ruralTex);
            textureState.ruralTex = textures::LoadTexture2D(path);
            textureState.useRural = (textureState.ruralTex != 0);
        }
        if (gui.ConsumeBuildingTextureClear(road::LotZone::Rural))
        {
            textures::SafeDeleteTexture(textureState.ruralTex);
            textureState.useRural = false;
        }
    }

    // handles GUI requests for urban, suburban and rural roof textures
    void HandleRoofTextureRequests(
        Gui &gui,
        TextureContext &textureState)
    {
        std::string path;

        if (gui.ConsumeRoofTextureApply(road::LotZone::Urban, path))
        {
            textures::SafeDeleteTexture(textureState.roofUrbanTex);
            textureState.roofUrbanTex = textures::LoadTexture2D(path);
            textureState.useRoofUrban = (textureState.roofUrbanTex != 0);
        }
        if (gui.ConsumeRoofTextureClear(road::LotZone::Urban))
        {
            textures::SafeDeleteTexture(textureState.roofUrbanTex);
            textureState.useRoofUrban = false;
        }

        if (gui.ConsumeRoofTextureApply(road::LotZone::Suburban, path))
        {
            textures::SafeDeleteTexture(textureState.roofSuburbanTex);
            textureState.roofSuburbanTex = textures::LoadTexture2D(path);
            textureState.useRoofSuburban = (textureState.roofSuburbanTex != 0);
        }
        if (gui.ConsumeRoofTextureClear(road::LotZone::Suburban))
        {
            textures::SafeDeleteTexture(textureState.roofSuburbanTex);
            textureState.useRoofSuburban = false;
        }

        if (gui.ConsumeRoofTextureApply(road::LotZone::Rural, path))
        {
            textures::SafeDeleteTexture(textureState.roofRuralTex);
            textureState.roofRuralTex = textures::LoadTexture2D(path);
            textureState.useRoofRural = (textureState.roofRuralTex != 0);
        }
        if (gui.ConsumeRoofTextureClear(road::LotZone::Rural))
        {
            textures::SafeDeleteTexture(textureState.roofRuralTex);
            textureState.useRoofRural = false;
        }
    }
}