#include "ShaderProgram.h"

#include "Shader.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <iostream>

ShaderProgram::~ShaderProgram() {
    Destroy();
}

bool ShaderProgram::Load(const char* vertexShaderPath, const char* fragmentShaderPath) {
    Destroy();

    m_programId = glCreateProgram();

    bool ready = false;

    {
        Shader vertexShader{GL_VERTEX_SHADER, vertexShaderPath};
        Shader fragmentShader{GL_FRAGMENT_SHADER, fragmentShaderPath};

        const bool vertexCompiled = vertexShader.Compile();
        const bool fragmentCompiled = fragmentShader.Compile();

        if (vertexCompiled && fragmentCompiled)
        {
            glAttachShader(m_programId, vertexShader.GetShaderId());
            glAttachShader(m_programId, fragmentShader.GetShaderId());

            glLinkProgram(m_programId);
            ready = CheckLink();
        }
    }

    if (!ready)
    {
        Destroy();
        return false;
    }

    return true;
}

void ShaderProgram::Use() const {
    glUseProgram(m_programId);
}

void ShaderProgram::Destroy() {
    if (m_programId != 0)
    {
        glDeleteProgram(m_programId);
        m_programId = 0;
    }
}

void ShaderProgram::SetMat4(const char* uniformName, const glm::mat4& value) const {
    const int location = glGetUniformLocation(m_programId, uniformName);

    if (location == -1)
    {
        std::cout << "Uniform not found: "
                  << uniformName
                  << '\n';

        return;
    }

    glUniformMatrix4fv(
        location,
        1,
        GL_FALSE,
        glm::value_ptr(value)
    );
}

void ShaderProgram::SetVec3(const char* uniformName, const glm::vec3& value) const {
    const int location = glGetUniformLocation(m_programId, uniformName);

    if (location == -1)
    {
        std::cout << "Uniform not found: "
                  << uniformName
                  << '\n';

        return;
    }

    glUniform3fv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::SetVec4(const char* uniformName, const glm::vec4& value) const {
    const int location = glGetUniformLocation(m_programId, uniformName);

    if (location == -1)
    {
        return;
    }

    glUniform4fv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::SetFloat(const char* uniformName, const float value) const {
    const int location = glGetUniformLocation(m_programId, uniformName);

    if (location == -1)
    {
        std::cout << "Uniform not found: "
                  << uniformName
                  << '\n';

        return;
    }

    glUniform1f(location, value);
}

void ShaderProgram::SetInt(const char* uniformName, const int value) const {
    const int location = glGetUniformLocation(m_programId, uniformName);

    if (location == -1)
    {
        return;
    }

    glUniform1i(location, value);
}

void ShaderProgram::SetOpacity(const float opacity) const {
    SetFloat("uOpacity", std::clamp(opacity, 0.0f, 1.0f));
}

ShaderProgramId ShaderProgram::GetProgramId() const {
    return m_programId;
}

bool ShaderProgram::CheckLink() const {
    int success{};
    glGetProgramiv(m_programId, GL_LINK_STATUS, &success);

    if (!success)
    {
        char infoLog[512]{};
        glGetProgramInfoLog(m_programId, sizeof(infoLog), nullptr, infoLog);

        std::cout << "Shader program link error:\n"
                  << infoLog
                  << '\n';

        return false;
    }

    return true;
}
