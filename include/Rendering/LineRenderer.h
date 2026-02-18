#pragma once 
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader;

namespace road 
{
    //main functionality is to draw lines
    class LineRenderer
    {
    private:
        GLuint mVAO = 0;
        GLuint mVBO = 0;
        std::size_t mVertexCount = 0;

    public:
        bool Initialize_Road();
        void Shutdown_Road();

        //uploads line list to GPU
        void Upload(const std::vector<glm::vec3> &lineVerts);

        //draws uploaded lines
        void Draw(const Shader& shader, const glm::mat4 &vp) const;

        //getter
        std::size_t VertexCount() const { return mVertexCount; }
    };

}