#pragma once

#include <glad/glad.h>
#include <string>

namespace textures
{
    GLuint LoadTexture2D(const std::string &path);
    void SafeDeleteTexture(GLuint &tex);
}