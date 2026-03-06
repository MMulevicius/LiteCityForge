#include "Core/Primitives.h"
#include <glm/gtc/type_ptr.hpp>
#include <Rendering/Shader.h>

// destructor to free GPU buffers
Primitives::~Primitives()
{
    Shutdown_Prim();
}

//creates ground VAO/VBO and uploads initial vertices
bool Primitives::Initialize_Prim()
{
    CreateGround();
    return (mGroundVAO != 0);
}

//if the buffer/array exist, delete them from GPU memory
void Primitives::Shutdown_Prim()
{
    if (mGroundVBO) glDeleteBuffers(1, &mGroundVBO);
    if (mGroundVAO) glDeleteVertexArrays(1, &mGroundVAO);


    mGroundVAO = mGroundVBO = 0;

}

void Primitives::CreateGround()
{
    const float verts[] = {
      // pos                // color
        -50.f, 0.f, -50.f,    0.5f, 0.5f, 0.5f,
         50.f, 0.f, -50.f,    0.5f, 0.5f, 0.5f,
         50.f, 0.f,  50.f,    0.5f, 0.5f, 0.5f,

        -50.f, 0.f, -50.f,    0.5f, 0.5f, 0.5f,
         50.f, 0.f,  50.f,    0.5f, 0.5f, 0.5f,
        -50.f, 0.f,  50.f,    0.5f, 0.5f, 0.5f,  

    };

    //asigns IDs for VAO/VBO and stores them
    glGenVertexArrays(1, &mGroundVAO);
    glGenBuffers(1, &mGroundVBO);

    //bind VAO, VBO and allocated GPU memory
    glBindVertexArray(mGroundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mGroundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    //attribute 0: 3 floats, offset 0, stride 6
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    ////attribute 1: 3 floats, offset 3 floats (skips position)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

}


//allows dynamic resizing
void Primitives::DrawGround(Shader& shader, const glm::mat4& viewProjection, const glm::vec2& centerXZ,
                            float halfWidth, float halfHeight)
{
    shader.use();

    // send viewProjection matrix to shader uniform "uVP"
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uVP"), 1, GL_FALSE, glm::value_ptr(viewProjection));

    //model matrix so that ground is in world space
    glm::mat4 model(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uModel"), 1, GL_FALSE, glm::value_ptr(model));

    //color
    glUniform3f(glGetUniformLocation(shader.ID, "uColor"), 0.40f, 0.52f, 0.43f);

    //city center + radius 
    float x0 = centerXZ.x - halfWidth;
    float x1 = centerXZ.x + halfWidth;
    float z0 = centerXZ.y - halfHeight;
    float z1 = centerXZ.y + halfHeight;


    //vertex array for resizing
    const float verts[] = {
        // pos                 // color 
        x0, 0.f, z0,           0.5f, 0.5f, 0.5f,
        x1, 0.f, z0,           0.5f, 0.5f, 0.5f,
        x1, 0.f, z1,           0.5f, 0.5f, 0.5f,

        x0, 0.f, z0,           0.5f, 0.5f, 0.5f,
        x1, 0.f, z1,           0.5f, 0.5f, 0.5f,
        x0, 0.f, z1,           0.5f, 0.5f, 0.5f
    };

    //replaces the existing buffers 
    glBindBuffer(GL_ARRAY_BUFFER, mGroundVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

    //binds ground VAO, draws 6 vertices as triangles and then unbinds VAO
    glBindVertexArray(mGroundVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

