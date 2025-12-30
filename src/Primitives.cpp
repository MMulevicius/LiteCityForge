#include "Primitives.h"
#include <glm/gtc/type_ptr.hpp>
#include <Shader.h>

Primitives::~Primitives()
{
    Shutdown_Prim();
}

bool Primitives::Initialize_Prim()
{
    CreateGround();
    CreateCube();
    return (mGroundVAO != 0 && mCubeVAO != 0);
}

void Primitives::Shutdown_Prim()
{
    if (mGroundVBO) glDeleteBuffers(1, &mGroundVBO);
    if (mGroundVAO) glDeleteVertexArrays(1, &mGroundVAO);
    if (mCubeVBO) glDeleteBuffers(1, &mCubeVBO);
    if (mCubeVAO) glDeleteVertexArrays(1, &mCubeVAO);

    mGroundVAO = mGroundVBO = 0;
    mCubeVAO = mCubeVBO = 0;
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

    glGenVertexArrays(1, &mGroundVAO);
    glGenBuffers(1, &mGroundVBO);

    glBindVertexArray(mGroundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mGroundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

}

void Primitives::CreateCube()
{
    const float v[] = {
        //pos                // colour
        //front
        -0.5f,-0.5f, 0.5f,   0.1f,0.1f,0.1f,
         0.5f,-0.5f, 0.5f,   0.1f,0.1f,0.1f,
         0.5f, 0.5f, 0.5f,   0.1f,0.1f,0.1f,
        -0.5f,-0.5f, 0.5f,   0.1f,0.1f,0.1f,
         0.5f, 0.5f, 0.5f,   0.1f,0.1f,0.1f,
        -0.5f, 0.5f, 0.5f,   0.1f,0.1f,0.1f,
        // back
        -0.5f,-0.5f,-0.5f,   0.1f,0.1f,0.1f,
        -0.5f, 0.5f,-0.5f,   0.1f,0.1f,0.1f,
         0.5f, 0.5f,-0.5f,   0.1f,0.1f,0.1f,
        -0.5f,-0.5f,-0.5f,   0.1f,0.1f,0.1f,
         0.5f, 0.5f,-0.5f,   0.1f,0.1f,0.1f,
         0.5f,-0.5f,-0.5f,   0.1f,0.1f,0.1f,
        // left
        -0.5f,-0.5f,-0.5f,   0.1f,0.1f,0.1f,
        -0.5f,-0.5f, 0.5f,   0.1f,0.1f,0.1f,
        -0.5f, 0.5f, 0.5f,   0.1f,0.1f,0.1f,
        -0.5f,-0.5f,-0.5f,   0.1f,0.1f,0.1f,
        -0.5f, 0.5f, 0.5f,   0.1f,0.1f,0.1f,
        -0.5f, 0.5f,-0.5f,   0.1f,0.1f,0.1f,
        // right
         0.5f,-0.5f,-0.5f,   0.1f,0.1f,0.1f,
         0.5f, 0.5f, 0.5f,   0.1f,0.1f,0.1f,
         0.5f,-0.5f, 0.5f,   0.1f,0.1f,0.1f,
         0.5f,-0.5f,-0.5f,   0.1f,0.1f,0.1f,
         0.5f, 0.5f,-0.5f,   0.1f,0.1f,0.1f,
         0.5f, 0.5f, 0.5f,   0.1f,0.1f,0.1f,
        // top
        -0.5f, 0.5f,-0.5f,   0.1f,0.1f,0.1f,
        -0.5f, 0.5f, 0.5f,   0.1f,0.1f,0.1f,
         0.5f, 0.5f, 0.5f,   0.1f,0.1f,0.1f,
        -0.5f, 0.5f,-0.5f,   0.1f,0.1f,0.1f,
         0.5f, 0.5f, 0.5f,   0.1f,0.1f,0.1f,
         0.5f, 0.5f,-0.5f,   0.1f,0.1f,0.1f,
        // bottom
        -0.5f,-0.5f,-0.5f,   0.1f,0.1f,0.1f,
         0.5f,-0.5f, 0.5f,   0.1f,0.1f,0.1f,
        -0.5f,-0.5f, 0.5f,   0.1f,0.1f,0.1f,
        -0.5f,-0.5f,-0.5f,   0.1f,0.1f,0.1f,
         0.5f,-0.5f,-0.5f,   0.1f,0.1f,0.1f,
         0.5f,-0.5f, 0.5f,   0.1f,0.1f,0.1f,
    };

    glGenVertexArrays(1, &mCubeVAO);
    glGenBuffers(1, &mCubeVBO);

    glBindVertexArray(mCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mCubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

}

void Primitives::DrawGround(Shader& shader,
                            const glm::mat4& vp,
                            const glm::vec2& centerXZ,
                            float halfWidth,
                            float halfHeight)
{
    shader.use();

    // set shader uniforms
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uVP"), 1, GL_FALSE, glm::value_ptr(vp));

    glm::mat4 model(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uModel"), 1, GL_FALSE, glm::value_ptr(model));

    glUniform3f(glGetUniformLocation(shader.ID, "uColor"), 0.6f, 0.6f, 0.6f);

    float x0 = centerXZ.x - halfWidth;
    float x1 = centerXZ.x + halfWidth;
    float z0 = centerXZ.y - halfHeight;
    float z1 = centerXZ.y + halfHeight;

    const float verts[] = {
        // pos                 // color 
        x0, 0.f, z0,           0.5f, 0.5f, 0.5f,
        x1, 0.f, z0,           0.5f, 0.5f, 0.5f,
        x1, 0.f, z1,           0.5f, 0.5f, 0.5f,

        x0, 0.f, z0,           0.5f, 0.5f, 0.5f,
        x1, 0.f, z1,           0.5f, 0.5f, 0.5f,
        x0, 0.f, z1,           0.5f, 0.5f, 0.5f
    };

    glBindBuffer(GL_ARRAY_BUFFER, mGroundVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

    glBindVertexArray(mGroundVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}



void Primitives::DrawCube(const Shader &shader, const glm::mat4 &vp, const glm::mat4 &model)
{
    shader.use();
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uVP"), 1, GL_FALSE, glm::value_ptr(vp));
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uModel"), 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(mCubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

}