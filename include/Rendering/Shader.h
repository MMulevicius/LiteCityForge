#pragma once

#include <glad/glad.h>
#include <string>

class Shader
{
public:
    //creates and compiles shader program for vertex and fragment shader files
    Shader(const char* vertexPath, const char* fragmentPath);

    //activate shader
    void use() const;
    //uniform setter
    void setFloat(const std::string& name, float value) const;
    //shader program ID
    unsigned int ID = 0;
};
