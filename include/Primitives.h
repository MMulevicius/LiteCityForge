#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader;


class Primitives
{
private:
    unsigned int mGroundVAO = 0, mGroundVBO = 0;

    void CreateGround();

public:
    Primitives () = default;
    ~Primitives();

    bool Initialize_Prim();
    void Shutdown_Prim();

    void DrawGround(Shader& shader,
                    const glm::mat4& viewProjection,
                    const glm::vec2& centerXZ,
                    float halfWidth,
                    float halfHeight);
};