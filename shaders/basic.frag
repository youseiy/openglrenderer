#version 330 core

in vec3 vColor;
in vec3 vNormal;
in vec3 vWorldPosition;
in vec2 vTexCoord;
in vec4 vTangent;

uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform vec3 uCameraPosition;
uniform vec4 uBaseColorFactor;
uniform vec3 uEmissiveFactor;
uniform float uAmbientStrength;
uniform float uLightIntensity;
uniform float uMetallic;
uniform float uRoughness;
uniform float uAmbientOcclusion;
uniform float uOcclusionStrength;
uniform float uNormalScale;
uniform float uAlphaCutoff;
uniform float uOpacity;
uniform int uAlphaMode;

uniform sampler2D uBaseColorMap;
uniform sampler2D uMetallicRoughnessMap;
uniform sampler2D uNormalMap;
uniform sampler2D uOcclusionMap;
uniform sampler2D uEmissiveMap;
uniform bool uHasBaseColorMap;
uniform bool uHasMetallicRoughnessMap;
uniform bool uHasNormalMap;
uniform bool uHasOcclusionMap;
uniform bool uHasEmissiveMap;

out vec4 FragColor;

const float PI = 3.14159265359;

float DistributionGGX(vec3 normal, vec3 halfway, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSquared = alpha * alpha;
    float normalDotHalfway = max(dot(normal, halfway), 0.0);
    float normalDotHalfwaySquared = normalDotHalfway * normalDotHalfway;
    float denominator = normalDotHalfwaySquared * (alphaSquared - 1.0) + 1.0;

    return alphaSquared / max(PI * denominator * denominator, 0.0001);
}

float GeometrySchlickGGX(float normalDotDirection, float roughness)
{
    float k = roughness + 1.0;
    k = (k * k) / 8.0;

    return normalDotDirection /
           max(normalDotDirection * (1.0 - k) + k, 0.0001);
}

float GeometrySmith(vec3 normal, vec3 viewDirection, vec3 lightDirection, float roughness)
{
    return GeometrySchlickGGX(max(dot(normal, viewDirection), 0.0), roughness) *
           GeometrySchlickGGX(max(dot(normal, lightDirection), 0.0), roughness);
}

vec3 FresnelSchlick(float cosineTheta, vec3 reflectanceAtNormal)
{
    return reflectanceAtNormal +
           (1.0 - reflectanceAtNormal) *
           pow(clamp(1.0 - cosineTheta, 0.0, 1.0), 5.0);
}

vec3 ToneMapACES(vec3 color)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;

    return clamp(
        (color * (a * color + b)) /
        (color * (c * color + d) + e),
        0.0,
        1.0
    );
}

vec3 GetNormal()
{
    vec3 normal = normalize(vNormal);
    // Double-sided materials need the normal to follow the visible face.
    if (!gl_FrontFacing)
    {
        normal = -normal;
    }
    if (!uHasNormalMap)
    {
        return normal;
    }

    // Re-orthogonalize the imported tangent before constructing the TBN basis.
    vec3 tangent = normalize(vTangent.xyz - normal * dot(normal, vTangent.xyz));
    vec3 bitangent = normalize(cross(normal, tangent)) * vTangent.w;
    vec3 mappedNormal = texture(uNormalMap, vTexCoord).xyz * 2.0 - 1.0;
    mappedNormal.xy *= uNormalScale;

    return normalize(mat3(tangent, bitangent, normal) * mappedNormal);
}

void main()
{
    vec4 baseColorSample = uHasBaseColorMap
        ? texture(uBaseColorMap, vTexCoord)
        : vec4(1.0);
    float alpha = uBaseColorFactor.a * baseColorSample.a * uOpacity;

    if (uAlphaMode == 1 && alpha < uAlphaCutoff)
    {
        discard;
    }

    vec3 sampledAlbedo = uHasBaseColorMap
        ? pow(max(baseColorSample.rgb, vec3(0.0)), vec3(2.2))
        : vec3(1.0);
    vec3 albedo = max(uBaseColorFactor.rgb * vColor * sampledAlbedo, vec3(0.0));
    // glTF packs roughness in G and metallic in B.
    vec4 metallicRoughnessSample = uHasMetallicRoughnessMap
        ? texture(uMetallicRoughnessMap, vTexCoord)
        : vec4(1.0);
    float metallic = clamp(uMetallic * metallicRoughnessSample.b, 0.0, 1.0);
    float roughness = clamp(uRoughness * metallicRoughnessSample.g, 0.045, 1.0);
    float occlusionSample = uHasOcclusionMap
        ? texture(uOcclusionMap, vTexCoord).r
        : 1.0;
    float ambientOcclusion = clamp(
        uAmbientOcclusion * mix(1.0, occlusionSample, uOcclusionStrength),
        0.0,
        1.0
    );

    vec3 normal = GetNormal();
    vec3 lightDirection = normalize(-uLightDirection);
    vec3 viewDirection = normalize(uCameraPosition - vWorldPosition);
    vec3 halfway = normalize(viewDirection + lightDirection);
    vec3 reflectanceAtNormal = mix(vec3(0.04), albedo, metallic);
    vec3 fresnel = FresnelSchlick(
        max(dot(halfway, viewDirection), 0.0),
        reflectanceAtNormal
    );
    float distribution = DistributionGGX(normal, halfway, roughness);
    float geometry = GeometrySmith(normal, viewDirection, lightDirection, roughness);
    vec3 specular = distribution * geometry * fresnel;
    specular /= max(
        4.0 * max(dot(normal, viewDirection), 0.0) *
        max(dot(normal, lightDirection), 0.0),
        0.0001
    );

    // Metals have no diffuse lobe; their base color contributes to specular reflection.
    vec3 diffuseWeight = (vec3(1.0) - fresnel) * (1.0 - metallic);
    vec3 radiance = uLightColor * max(uLightIntensity, 0.0);
    float normalDotLight = max(dot(normal, lightDirection), 0.0);
    vec3 directLighting =
        (diffuseWeight * albedo / PI + specular) *
        radiance *
        normalDotLight;
    vec3 ambientLighting =
        albedo * max(uAmbientStrength, 0.0) * ambientOcclusion;

    vec3 emissiveSample = uHasEmissiveMap
        ? pow(texture(uEmissiveMap, vTexCoord).rgb, vec3(2.2))
        : vec3(1.0);
    vec3 color = ambientLighting +
                 directLighting +
                 uEmissiveFactor * emissiveSample;

    color = ToneMapACES(color);
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, uAlphaMode == 2 ? alpha : 1.0);
}
