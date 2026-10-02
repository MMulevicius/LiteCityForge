#include "Rendering/Shader.h"

#include <fstream>
#include <sstream>
#include <iostream>

static bool CheckShaderErrors(GLuint object, const std::string &type)
{
    GLint success = GL_FALSE;
    char infoLog[1024]{};

    if (type == "PROGRAM")
    {
        glGetProgramiv(object, GL_LINK_STATUS, &success);

        if (success == GL_FALSE)
        {
            glGetProgramInfoLog(object, sizeof(infoLog), nullptr, infoLog);
            std::cout << "PROGRAM LINK ERROR:\n"
                      << infoLog << std::endl;
        }
    }
    else
    {
        glGetShaderiv(object, GL_COMPILE_STATUS, &success);

        if (success == GL_FALSE)
        {
            glGetShaderInfoLog(object, sizeof(infoLog), nullptr, infoLog);
            std::cout << type << " SHADER COMPILE ERROR:\n"
                      << infoLog << std::endl;
        }
    }

    return success == GL_TRUE;
}

Shader::Shader(const std::filesystem::path &vertexPath,
               const std::filesystem::path &fragmentPath)
{
    std::string vertexCode;
    std::string fragmentCode;

    try
    {
        std::ifstream vShaderFile(vertexPath);
        std::ifstream fShaderFile(fragmentPath);

        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        std::stringstream vStream;
        std::stringstream fStream;

        vStream << vShaderFile.rdbuf();
        fStream << fShaderFile.rdbuf();

        vertexCode = vStream.str();
        fragmentCode = fStream.str();
    }
    catch (const std::ifstream::failure &error)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: "
                  << error.what() << '\n';
        return;
    }

    const char *vShaderCode = vertexCode.c_str();
    const char *fShaderCode = fragmentCode.c_str();

    const GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    const GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);

    if (vertex == 0 || fragment == 0)
    {
        std::cout << "ERROR::SHADER::COULD_NOT_CREATE_SHADER\n";

        if (vertex != 0)
            glDeleteShader(vertex);

        if (fragment != 0)
            glDeleteShader(fragment);

        return;
    }

    glShaderSource(vertex, 1, &vShaderCode, nullptr);
    glCompileShader(vertex);
    const bool vertexCompiled = CheckShaderErrors(vertex, "VERTEX");

    glShaderSource(fragment, 1, &fShaderCode, nullptr);
    glCompileShader(fragment);
    const bool fragmentCompiled = CheckShaderErrors(fragment, "FRAGMENT");

    if (!vertexCompiled || !fragmentCompiled)
    {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return;
    }

    ID = glCreateProgram();

    if (ID == 0)
    {
        std::cout << "ERROR::SHADER::COULD_NOT_CREATE_PROGRAM\n";
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return;
    }

    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);

    const bool linked = CheckShaderErrors(ID, "PROGRAM");

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    if (!linked)
    {
        glDeleteProgram(ID);
        ID = 0;
    }
}

void Shader::use() const
{
    // activate this shader program for rendering
    glUseProgram(ID);
}

void Shader::setFloat(const std::string &name, float value) const
{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::Shutdown()
{
    if (ID != 0)
    {
        glDeleteProgram(ID);
        ID = 0;
    }
}
