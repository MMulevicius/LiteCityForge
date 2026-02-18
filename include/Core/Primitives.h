#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader;

// creates a ground mesh
class Primitives
{
private:
    
    unsigned int mGroundVAO = 0, mGroundVBO = 0;

    //creates a default ground plane of size 100x100, centered origin, 2 triangles 
    void CreateGround();

public:
    Primitives () = default;
    ~Primitives();

    //calls CreateGround()
    bool Initialize_Prim();

    //deletes buffers and resets IDs to 0
    void Shutdown_Prim();

    //resizes ground every draw.
    void DrawGround(Shader& shader,
                    const glm::mat4& viewProjection,
                    const glm::vec2& centerXZ,
                    float halfWidth,
                    float halfHeight);
};