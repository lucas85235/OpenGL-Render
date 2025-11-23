#ifndef CUSTOM_SHADERS_HPP
#define CUSTOM_SHADERS_HPP

namespace CustomShaders {

// ============================================
// VERTEX SHADER COM SUPORTE A TANGENTES
// ============================================
const char* AdvancedVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    vec3 Tangent;
    vec3 Bitangent;
    mat3 TBN;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.TexCoords = aTexCoords;
    
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vs_out.Normal = normalize(normalMatrix * aNormal);
    vs_out.Tangent = normalize(normalMatrix * aTangent);
    vs_out.Bitangent = normalize(normalMatrix * aBitangent);
    
    // TBN matrix for normal mapping
    vs_out.TBN = mat3(vs_out.Tangent, vs_out.Bitangent, vs_out.Normal);
    
    gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);
}
)";

// ============================================
// FRAGMENT SHADER COM MATERIAL CUSTOMIZADO
// ============================================
const char* CustomMaterialFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    vec3 Tangent;
    vec3 Bitangent;
    mat3 TBN;
} fs_in;

// Material properties
struct Material {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    vec3 emission;
    float emissionStrength;
};

uniform Material material;

// Texturas
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_metallic1;
uniform sampler2D texture_roughness1;
uniform sampler2D texture_ao1;
uniform sampler2D texture_emission1;

// Flags de presença de textura
uniform bool hasTextureDiffuse;
uniform bool hasTextureNormal;
uniform bool hasTextureMetallic;
uniform bool hasTextureRoughness;
uniform bool hasTextureAO;
uniform bool hasTextureEmission;

// Iluminação
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

const float PI = 3.14159265359;

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / denom;
}

// Geometry Function (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Fresnel Equation (Schlick approximation)
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    // Obter propriedades do material
    vec3 albedo = material.albedo;
    if (hasTextureDiffuse) {
        albedo = texture(texture_diffuse1, fs_in.TexCoords).rgb;
        albedo = pow(albedo, vec3(2.2)); // sRGB to linear
    }
    
    float metallic = material.metallic;
    if (hasTextureMetallic) {
        metallic = texture(texture_metallic1, fs_in.TexCoords).r;
    }
    
    float roughness = material.roughness;
    if (hasTextureRoughness) {
        roughness = texture(texture_roughness1, fs_in.TexCoords).r;
    }
    
    float ao = material.ao;
    if (hasTextureAO) {
        ao = texture(texture_ao1, fs_in.TexCoords).r;
    }
    
    // Normal mapping
    vec3 N = fs_in.Normal;
    if (hasTextureNormal) {
        vec3 normalMap = texture(texture_normal1, fs_in.TexCoords).rgb;
        normalMap = normalMap * 2.0 - 1.0;
        N = normalize(fs_in.TBN * normalMap);
    }
    
    vec3 V = normalize(viewPos - fs_in.FragPos);
    
    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // Reflectance equation
    vec3 Lo = vec3(0.0);
    
    // Point light
    vec3 L = normalize(lightPos - fs_in.FragPos);
    vec3 H = normalize(V + L);
    float distance = length(lightPos - fs_in.FragPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = lightColor * attenuation;
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    
    // Ambient lighting
    vec3 ambient = vec3(0.03) * albedo * ao;
    
    // Emission
    vec3 emission = material.emission * material.emissionStrength;
    if (hasTextureEmission) {
        emission += texture(texture_emission1, fs_in.TexCoords).rgb * material.emissionStrength;
    }
    
    vec3 color = ambient + Lo + emission;
    
    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}
)";

// ============================================
// SHADER SIMPLES COM PHONG
// ============================================
const char* SimplePhongFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    vec3 Tangent;
    vec3 Bitangent;
    mat3 TBN;
} fs_in;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

uniform Material material;
uniform sampler2D texture_diffuse1;
uniform bool hasTextureDiffuse;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

void main() {
    // Ambient
    vec3 ambient = material.ambient * lightColor;
    
    // Diffuse
    vec3 norm = normalize(fs_in.Normal);
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * material.diffuse * lightColor;
    
    // Specular
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = spec * material.specular * lightColor;
    
    vec3 result = ambient + diffuse + specular;
    
    if (hasTextureDiffuse) {
        result *= texture(texture_diffuse1, fs_in.TexCoords).rgb;
    }
    
    FragColor = vec4(result, 1.0);
}
)";

} // namespace CustomShaders

#endif // CUSTOM_SHADERS_HPP