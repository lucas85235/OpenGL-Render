#ifndef CUSTOM_SHADERS_HPP
#define CUSTOM_SHADERS_HPP

namespace CustomShaders {

const char* AdvancedVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out mat3 TBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    TexCoords = aTexCoords;
    
    // Matriz Normal
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = normalMatrix * aNormal;
    
    // TBN Matrix para Normal Mapping
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    TBN = mat3(T, B, N);
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

// Em src/renderer/custom_shaders.hpp

const char* CustomMaterialFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;

// --- ESTRUTURAS DE LUZ ---
struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float radius; // Usado para attenuation (opcional mas bom para performance)
};

#define MAX_POINT_LIGHTS 4 

// --- UNIFORMS ---
uniform DirectionalLight dirLight;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int numPointLights; // Quantas luzes ativas temos?

uniform vec3 viewPos;

// Material e Texturas (Mesmo de antes)
struct Material {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    vec3 emission;
    float emissionStrength;
};
uniform Material material;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_metallic1;
uniform sampler2D texture_roughness1;
uniform sampler2D texture_ao1;
uniform sampler2D texture_emission1;

uniform bool hasTextureDiffuse;
uniform bool hasTextureNormal;
uniform bool hasTextureMetallic;
uniform bool hasTextureRoughness;
uniform bool hasTextureAO;
uniform bool hasTextureEmission;

const float PI = 3.14159265359;

// --- FUNÇÕES PBR (Distribuição, Geometria, Fresnel) ---
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / max(denom, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// --- CÁLCULO DE UMA ÚNICA LUZ PBR ---
vec3 CalcPBRLight(vec3 L, vec3 V, vec3 N, vec3 F0, vec3 albedo, float metallic, float roughness, vec3 radiance) {
    vec3 H = normalize(V + L);
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);   
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
       
    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;	  

    float NdotL = max(dot(N, L), 0.0);        

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main() {
    // 1. Ler Propriedades do Material
    vec3 albedo = material.albedo;
    if (hasTextureDiffuse) albedo = texture(texture_diffuse1, TexCoords).rgb;
    
    float metallic = material.metallic;
    if (hasTextureMetallic) metallic = texture(texture_metallic1, TexCoords).r;
    
    float roughness = material.roughness;
    if (hasTextureRoughness) roughness = texture(texture_roughness1, TexCoords).r;
    
    float ao = material.ao;
    if (hasTextureAO) ao = texture(texture_ao1, TexCoords).r;

    // Normal Mapping
    vec3 N = normalize(Normal);
    if (hasTextureNormal) {
        N = texture(texture_normal1, TexCoords).rgb;
        N = N * 2.0 - 1.0;
        N = normalize(TBN * N);
    }
    vec3 V = normalize(viewPos - FragPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // --- LOOP DE ILUMINAÇÃO ---
    vec3 Lo = vec3(0.0);

    // 1. Luz Direcional (Sol)
    {
        vec3 L = normalize(-dirLight.direction);
        vec3 radiance = dirLight.color * dirLight.intensity;
        Lo += CalcPBRLight(L, V, N, F0, albedo, metallic, roughness, radiance);
    }

    // 2. Luzes Pontuais (Lâmpadas)
    for(int i = 0; i < numPointLights; ++i) {
        vec3 L = normalize(pointLights[i].position - FragPos);
        float distance = length(pointLights[i].position - FragPos);
        
        // Inverse Square Law (Física Real)
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = pointLights[i].color * pointLights[i].intensity * attenuation;

        Lo += CalcPBRLight(L, V, N, F0, albedo, metallic, roughness, radiance);
    }

    // 3. Ambiente + Emissão
    vec3 ambient = vec3(0.03) * albedo * ao; // IBL virá aqui depois
    
    vec3 emission = vec3(0.0);
    if (hasTextureEmission) emission = texture(texture_emission1, TexCoords).rgb;
    else emission = material.emission;
    emission *= material.emissionStrength;

    vec3 color = ambient + Lo + emission;

    // HDR Tonemapping & Gamma
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}
)";

} // namespace CustomShaders

#endif // CUSTOM_SHADERS_HPP
