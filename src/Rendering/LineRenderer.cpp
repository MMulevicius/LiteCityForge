#include "Rendering/LineRenderer.h"
#include <glm/gtc/type_ptr.hpp>
#include "Rendering/Shader.h"

namespace road
{
    //intializes and binds the VAO and VBO
    bool LineRenderer::Initialize_Road()
    {
        glGenVertexArrays(1, &mVAO);
        glGenBuffers(1, &mVBO);

        glBindVertexArray(mVAO);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);

        //position only 
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        return mVAO != 0 && mVBO != 0;
    }
    //resets VAO and VBO
    void LineRenderer::Shutdown_Road()
    {
        if (mVBO) glDeleteBuffers(1, &mVBO);
        if (mVAO) glDeleteVertexArrays(1, &mVAO);
        mVBO = 0;
        mVAO = 0;
        mVertexCount = 0;
    }

    void LineRenderer::Upload(const std::vector<glm::vec3> &lineVerts)
    {
        mVertexCount = lineVerts.size();

        //selects VBO, allocates GPU memory and uploads data
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBufferData(GL_ARRAY_BUFFER,
                    (GLsizeiptr)(lineVerts.size() * sizeof(glm::vec3)),
                    lineVerts.data(),
                    GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    //sets up uVP and uModel using uniform calls
    void LineRenderer::Draw(const Shader &shader, const glm::mat4 &vp) const
    {
        if (mVertexCount == 0) return;

        shader.use();
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uVP"), 1, GL_FALSE, glm::value_ptr(vp));

        glm::mat4 model(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uModel"), 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(mVAO);
        glDrawArrays(GL_LINES, 0, (GLsizei)mVertexCount);
        glBindVertexArray(0);
    }


}