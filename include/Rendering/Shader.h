#pragma once

#include <glad/glad.h>
#include <string>
#include <filesystem>

class Shader
{
public:
    // creates and compiles shader program for vertex and fragment shader files
    Shader(const std::filesystem::path &vertexPath,
           const std::filesystem::path &fragmentPath);

    // activate shader
    void use() const;
    // uniform setter
    void setFloat(const std::string &name, float value) const;
    // shader program ID
    unsigned int ID = 0;

    void Shutdown();
};
