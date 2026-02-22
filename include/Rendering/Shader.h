#pragma once

#include <glad/glad.h>
#include <string>

class Shader
{
public:
    Shader(const char* vertexPath, const char* fragmentPath);

    void use() const;
    void setFloat(const std::string& name, float value) const;

    unsigned int ID = 0;
};
