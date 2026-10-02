#include "Textures/TextureRequests.h"
#include "Textures/TextureUtils.h"
#include <iostream>

namespace textures
{
    // handles GUI requests for applying or clearing the window texture
    void HandleWindowTextureRequests(Gui &gui, GLuint &windowTex, bool &useWindowTex)
    {
        std::string path;

        if (gui.ConsumeWindowTextureApply(path))
        {
            textures::SafeDeleteTexture(windowTex);
            windowTex = textures::LoadTexture2D(path);
            useWindowTex = (windowTex != 0);
        }

        if (gui.ConsumeWindowTextureClear())
        {
            textures::SafeDeleteTexture(windowTex);
            useWindowTex = false;
        }
    }
    // handles GUI requests for road, sidewlak and ground textures
    void HandleSceneTextureRequests(Gui &gui,
                                    GLuint &roadTex, bool &useRoad,
                                    GLuint &sidewalkTex, bool &useSidewalk,
                                    GLuint &groundTex, bool &useGround)
    {
        std::string path;

        if (gui.ConsumeRoadTextureApply(path))
        {
            textures::SafeDeleteTexture(roadTex);
            roadTex = textures::LoadTexture2D(path);
            useRoad = (roadTex != 0);
        }
        if (gui.ConsumeRoadTextureClear())
        {
            textures::SafeDeleteTexture(roadTex);
            useRoad = false;
        }

        if (gui.ConsumeSidewalkTextureApply(path))
        {
            textures::SafeDeleteTexture(sidewalkTex);
            sidewalkTex = textures::LoadTexture2D(path);
            useSidewalk = (sidewalkTex != 0);
        }
        if (gui.ConsumeSidewalkTextureClear())
        {
            textures::SafeDeleteTexture(sidewalkTex);
            useSidewalk = false;
        }

        if (gui.ConsumeGroundTextureApply(path))
        {
            textures::SafeDeleteTexture(groundTex);
            groundTex = textures::LoadTexture2D(path);
            useGround = (groundTex != 0);
        }
        if (gui.ConsumeGroundTextureClear())
        {
            textures::SafeDeleteTexture(groundTex);
            useGround = false;
        }
    }

    // handles GUI requests for urban, suburban and rural building textures
    void HandleBuildingTextureRequests(
        Gui &gui,
        GLuint &urbanTex, GLuint &suburbanTex, GLuint &ruralTex,
        bool &useUrban, bool &useSuburban, bool &useRural)
    {
        std::string path;

        // urban
        if (gui.ConsumeBuildingTextureApply(road::LotZone::Urban, path))
        {
            std::cout << "Loaded urbanTex=" << urbanTex << "\n";
            textures::SafeDeleteTexture(urbanTex);
            urbanTex = textures::LoadTexture2D(path);
            useUrban = (urbanTex != 0);
        }
        if (gui.ConsumeBuildingTextureClear(road::LotZone::Urban))
        {
            textures::SafeDeleteTexture(urbanTex);
            useUrban = false;
        }

        // suburban
        if (gui.ConsumeBuildingTextureApply(road::LotZone::Suburban, path))
        {
            textures::SafeDeleteTexture(suburbanTex);
            suburbanTex = textures::LoadTexture2D(path);
            useSuburban = (suburbanTex != 0);
        }
        if (gui.ConsumeBuildingTextureClear(road::LotZone::Suburban))
        {
            textures::SafeDeleteTexture(suburbanTex);
            useSuburban = false;
        }

        // rural
        if (gui.ConsumeBuildingTextureApply(road::LotZone::Rural, path))
        {
            textures::SafeDeleteTexture(ruralTex);
            ruralTex = textures::LoadTexture2D(path);
            useRural = (ruralTex != 0);
        }
        if (gui.ConsumeBuildingTextureClear(road::LotZone::Rural))
        {
            textures::SafeDeleteTexture(ruralTex);
            useRural = false;
        }
    }

    // handles GUI requests for urban, suburban and rural roof textures
    void HandleRoofTextureRequests(
        Gui &gui,
        GLuint &urbanRoof, GLuint &suburbanRoof, GLuint &ruralRoof,
        bool &useUrbanRoof, bool &useSuburbanRoof, bool &useRuralRoof)
    {
        std::string path;

        if (gui.ConsumeRoofTextureApply(road::LotZone::Urban, path))
        {
            textures::SafeDeleteTexture(urbanRoof);
            urbanRoof = textures::LoadTexture2D(path);
            useUrbanRoof = (urbanRoof != 0);
        }
        if (gui.ConsumeRoofTextureClear(road::LotZone::Urban))
        {
            textures::SafeDeleteTexture(urbanRoof);
            useUrbanRoof = false;
        }

        if (gui.ConsumeRoofTextureApply(road::LotZone::Suburban, path))
        {
            textures::SafeDeleteTexture(suburbanRoof);
            suburbanRoof = textures::LoadTexture2D(path);
            useSuburbanRoof = (suburbanRoof != 0);
        }
        if (gui.ConsumeRoofTextureClear(road::LotZone::Suburban))
        {
            textures::SafeDeleteTexture(suburbanRoof);
            useSuburbanRoof = false;
        }

        if (gui.ConsumeRoofTextureApply(road::LotZone::Rural, path))
        {
            textures::SafeDeleteTexture(ruralRoof);
            ruralRoof = textures::LoadTexture2D(path);
            useRuralRoof = (ruralRoof != 0);
        }
        if (gui.ConsumeRoofTextureClear(road::LotZone::Rural))
        {
            textures::SafeDeleteTexture(ruralRoof);
            useRuralRoof = false;
        }
    }
}