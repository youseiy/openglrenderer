#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aColor;
layout (location = 4) in vec4 aTangent;

out vec3 vColor;
out vec3 vNormal;
out vec3 vWorldPosition;
out vec2 vTexCoord;
out vec4 vTangent;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    vec4 worldPosition = uModel * vec4(aPos, 1.0);

    vColor = aColor;
    // Inverse-transpose preserves normals under non-uniform model scaling.
    mat3 normalMatrix = mat3(transpose(inverse(uModel)));
    vNormal = normalize(normalMatrix * aNormal);
    vWorldPosition = worldPosition.xyz;
    vTexCoord = aTexCoord;
    vTangent = vec4(normalize(mat3(uModel) * aTangent.xyz), aTangent.w);
    gl_Position = uProjection * uView * worldPosition;
}
