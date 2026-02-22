#pragma once
#include <string>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"
#include "Core/Camera.h"

class Skybox
{
public:
    Skybox() = default;
    ~Skybox() = default;

    bool Initialize(const std::string& directory);
    void Shutdown();

    // draw only in both 3D and 2D
    void Draw(const Camera& camera, float aspect, bool is3D);

private:
    GLuint mVAO = 0;
    GLuint mVBO = 0;
    GLuint mCubemapTex = 0;

    Shader* mShader = nullptr;

    GLuint LoadCubemap(const std::vector<std::string>& faces);
};
