#pragma once

#include "GUI/Gui.h"
#include <glad/glad.h>

namespace textures
{
    void HandleWindowTextureRequests(
        Gui &gui,
        GLuint &windowTex,
        bool &useWindowTex);

    void HandleSceneTextureRequests(Gui &gui,
                                    GLuint &roadTex, bool &useRoad,
                                    GLuint &sidewalkTex, bool &useSidewalk,
                                    GLuint &groundTex, bool &useGround);

    void HandleBuildingTextureRequests(
        Gui &gui,
        GLuint &urbanTex, GLuint &suburbanTex, GLuint &ruralTex,
        bool &useUrban, bool &useSuburban, bool &useRural);

    void HandleRoofTextureRequests(
        Gui &gui,
        GLuint &urbanRoof, GLuint &suburbanRoof, GLuint &ruralRoof,
        bool &useUrbanRoof, bool &useSuburbanRoof, bool &useRuralRoof);
}