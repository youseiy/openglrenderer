//
// Created by yousei on 27/05/2026.
//

#include "Shader.h"

#include "glad/glad.h"

#include <fstream>
#include <iostream>
#include <sstream>



Shader::Shader(const ShaderType type, const char* filePath)  :  m_filePath{filePath}, m_shaderId(glCreateShader(type)), m_shaderType{type}
{
    if (LoadSourceFromFile(filePath))
    {
        const char* source = m_sourceCode.c_str();

        glShaderSource(
            m_shaderId,
            1,
            &source,
            nullptr
        );
    }
}


Shader::~Shader() {
    if (m_shaderId != 0)
    {
        glDeleteShader(m_shaderId);
    }
}

bool Shader::Compile() const {
    if (m_sourceCode.empty())
    {
        std::cout << GetShaderTypeName()
                  << " shader source is empty: "
                  << m_filePath
                  << '\n';

        return false;
    }

    glCompileShader(m_shaderId);

    int success{};
    glGetShaderiv(m_shaderId, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char infoLog[512]{};
        glGetShaderInfoLog(m_shaderId, sizeof(infoLog), nullptr, infoLog);

        std::cout << GetShaderTypeName()
                  << " shader compile error in "
                  << m_filePath
                  << ":\n"
                  << infoLog
                  << '\n';

        return false;
    }

    return true;
}

ShaderId Shader::GetShaderId() const {
    return m_shaderId;
}


bool Shader::LoadSourceFromFile(const char* filePath) {
    std::ifstream file{filePath};

    if (!file)
    {
        std::cout << GetShaderTypeName()
                  << " shader file not found: "
                  << filePath
                  << '\n';

        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    m_sourceCode = buffer.str();

    return true;
}


const char* Shader::GetShaderTypeName() const {
    switch (m_shaderType)
    {
        case GL_VERTEX_SHADER:
            return "Vertex";
        case GL_FRAGMENT_SHADER:
            return "Fragment";
        default:
            return "Unknown";
    }
}
