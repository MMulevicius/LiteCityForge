#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {

private:

    //checks for shader compile status or program link status
    void checkErrors(unsigned int object, const std::string& type) {
        int success;
        char infoLog[1024];

        if (type == "PROGRAM") {
            glGetProgramiv(object, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(object, 1024, nullptr, infoLog);
                std::cout << "PROGRAM LINK ERROR:\n" << infoLog << std::endl;
            }
        } else {
            glGetShaderiv(object, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(object, 1024, nullptr, infoLog);
                std::cout << type << " SHADER COMPILE ERROR:\n"
                          << infoLog << std::endl;
            }
        }
    }

public:
    //program object handle
    unsigned int ID;


    //loads, compiles and links shader files
    Shader(const char* vertexPath, const char* fragmentPath) {
        
        //file streams
        std::ifstream vShaderFile, fShaderFile;
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        std::string vertexCode, fragmentCode;

        //reads .vert and .frag files
        try {
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);

            std::stringstream vStream, fStream;
            vStream << vShaderFile.rdbuf();
            fStream << fShaderFile.rdbuf();

            vShaderFile.close();
            fShaderFile.close();

            //stores extracted GLSL code from .vert and .frag
            vertexCode   = vStream.str();
            fragmentCode = fStream.str();
        } catch (std::ifstream::failure& e) {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ\n";
        }

        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        //containers for OpenGL shader objects
        unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
        unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);

        //attaches source code and compiles the shader into GPU understandable form
        glShaderSource(vertex, 1, &vShaderCode, nullptr);
        glCompileShader(vertex);
        checkErrors(vertex, "VERTEX");

        glShaderSource(fragment, 1, &fShaderCode, nullptr);
        glCompileShader(fragment);
        checkErrors(fragment, "FRAGMENT");

        //creates an OpenGL program object, attaches the shader object and links them to a final GPU program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkErrors(ID, "PROGRAM");

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    //activates shader programs (mainly for draw calls)
    void use() const {
        glUseProgram(ID);
    }

    //sets a uniform float, usually called after use()
    void setFloat(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }


};

#endif
