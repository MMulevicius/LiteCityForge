#include "Rendering/Shader.h"

#include <fstream>
#include <sstream>
#include <iostream>

static void CheckShaderErrors(unsigned int object, const std::string& type)
{
    int success = 0;
    char infoLog[1024];

    if (type == "PROGRAM")
    {
        glGetProgramiv(object, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(object, 1024, nullptr, infoLog);
            std::cout << "PROGRAM LINK ERROR:\n" << infoLog << std::endl;
        }
    }
    else
    {
        glGetShaderiv(object, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(object, 1024, nullptr, infoLog);
            std::cout << type << " SHADER COMPILE ERROR:\n" << infoLog << std::endl;
        }
    }
}

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    //read shader source files into strings
    std::ifstream vShaderFile, fShaderFile;
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    //load vertex and fragment shader source code from files
    std::string vertexCode, fragmentCode;

    try
    {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);

        std::stringstream vStream, fStream;
        vStream << vShaderFile.rdbuf();
        fStream << fShaderFile.rdbuf();

        vShaderFile.close();
        fShaderFile.close();

        vertexCode = vStream.str();
        fragmentCode = fStream.str();
    }
    catch (std::ifstream::failure&)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ\n";
    }

    //compile vertex and fragment shaders
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vertex, 1, &vShaderCode, nullptr);
    glCompileShader(vertex);
    CheckShaderErrors(vertex, "VERTEX");

    glShaderSource(fragment, 1, &fShaderCode, nullptr);
    glCompileShader(fragment);
    CheckShaderErrors(fragment, "FRAGMENT");

    //link compiled shader into a GPU shader program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    CheckShaderErrors(ID, "PROGRAM");

    //delete individual shaders after linking
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::use() const
{
    //activate this shader program for rendering
    glUseProgram(ID);
}

void Shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}
