#pragma once

#include <glm/glm.hpp>

struct CityContext
{
    float cityR = 0.0f;
    glm::vec2 centerXZ = glm::vec2(0.0f, 0.0f);
};

struct GroundContext
{
    float margin = 20.0f;
    float halfW = 0.0f;
    float halfH = 0.0f;
};

struct FrameContext
{
    float deltaTime = 0.0f;
    int w = 1;
    int h = 1;
    float aspect = 1.0f;
    glm::mat4 viewProjection = glm::mat4(1.0f);
};
