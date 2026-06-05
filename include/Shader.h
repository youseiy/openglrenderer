//
// Created by yousei on 27/05/2026.
//

#ifndef GLRENDERER_SHADER_H
#define GLRENDERER_SHADER_H

#include <string>

using ShaderType = unsigned int;

using ShaderId = unsigned int;

class Shader {

public:
    Shader()=delete;

    Shader(ShaderType type, const char *filePath);

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    [[nodiscard]] bool Compile() const;

    ~Shader();


    [[nodiscard]] ShaderId GetShaderId() const;


private:
    [[nodiscard]] bool LoadSourceFromFile(const char* filePath);
    [[nodiscard]] const char* GetShaderTypeName() const;

    std::string m_filePath{};
    std::string m_sourceCode{};

    ShaderId m_shaderId{};

    ShaderType m_shaderType{};
};


#endif //GLRENDERER_SHADER_H
