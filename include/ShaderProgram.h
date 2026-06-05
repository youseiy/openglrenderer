#ifndef GLRENDERER_SHADERPROGRAM_H
#define GLRENDERER_SHADERPROGRAM_H

#include <glm/glm.hpp>

using ShaderProgramId = unsigned int;

class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    [[nodiscard]] bool Load(const char* vertexShaderPath, const char* fragmentShaderPath);
    void Use() const;
    void Destroy();

    void SetMat4(const char* uniformName, const glm::mat4& value) const;
    void SetVec3(const char* uniformName, const glm::vec3& value) const;
    void SetVec4(const char* uniformName, const glm::vec4& value) const;
    void SetFloat(const char* uniformName, float value) const;
    void SetInt(const char* uniformName, int value) const;
    void SetOpacity(float opacity) const;

    [[nodiscard]] ShaderProgramId GetProgramId() const;

private:
    [[nodiscard]] bool CheckLink() const;

    ShaderProgramId m_programId{};
};

#endif //GLRENDERER_SHADERPROGRAM_H
