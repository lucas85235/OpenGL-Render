#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// Light structures
struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float radius;
};

#define MAX_POINT_LIGHTS 4

// Uniforms
uniform DirectionalLight dirLight;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int numPointLights;
uniform vec3 viewPos;

// Material
struct Material {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    vec3 emission;
    float emissionStrength;
};
uniform Material material;

// Textures
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

// IBL
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;
uniform bool useIBL;

const float PI = 3.14159265359;

// PBR Functions
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = max(roughness * roughness, 0.001);
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k + 0.0001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0001);
    float NdotL = max(dot(N, L), 0.0001);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

vec3 CalcPBRLight(vec3 L, vec3 V, vec3 N, vec3 F0, vec3 albedo, float metallic, float roughness, vec3 radiance) {
    vec3 H = normalize(V + L);
    
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 numerator = NDF * G * F;
    float NdotV = max(dot(N, V), 0.0001);
    float NdotL = max(dot(N, L), 0.0001);
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main() {
    vec2 uv = TexCoords;
    
    // Material properties
    vec3 albedo = material.albedo;
    if (hasTextureDiffuse) {
        vec4 texColor = texture(texture_diffuse1, uv);
        albedo = pow(texColor.rgb, vec3(2.2));
    }
    
    float metallic = material.metallic;
    if (hasTextureMetallic) metallic = texture(texture_metallic1, uv).r;
    
    float roughness = clamp(material.roughness, 0.04, 1.0);
    if (hasTextureRoughness) roughness = clamp(texture(texture_roughness1, uv).r, 0.04, 1.0);
    
    float ao = material.ao;
    if (hasTextureAO) ao = texture(texture_ao1, uv).r;
    
    // Normal
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    
    // Fresnel base
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    
    // Direct lighting
    vec3 Lo = vec3(0.0);
    
    // Directional light
    {
        vec3 L = normalize(-dirLight.direction);
        vec3 radiance = dirLight.color * dirLight.intensity;
        Lo += CalcPBRLight(L, V, N, F0, albedo, metallic, roughness, radiance);
    }
    
    // Point lights
    for (int i = 0; i < numPointLights; ++i) {
        vec3 L = normalize(pointLights[i].position - FragPos);
        float distance = length(pointLights[i].position - FragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = pointLights[i].color * pointLights[i].intensity * attenuation;
        Lo += CalcPBRLight(L, V, N, F0, albedo, metallic, roughness, radiance);
    }
    
    // Ambient / IBL
    vec3 ambient = vec3(0.0);
    float NdotV = max(dot(N, V), 0.0001);
    
    if (useIBL) {
        vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
        vec3 kD = (1.0 - F) * (1.0 - metallic);
        
        vec3 irradiance = texture(irradianceMap, N).rgb;
        vec3 diffuse = kD * irradiance * albedo;
        
        vec3 R = reflect(-V, N);
        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
        vec2 envBRDF = texture(brdfLUT, vec2(NdotV, roughness)).rg;
        vec3 specular = prefilteredColor * (F0 * envBRDF.x + envBRDF.y);
        
        ambient = (diffuse + specular) * ao;
    } else {
        ambient = vec3(0.03) * albedo * ao;
    }
    
    // Emission
    vec3 emission = vec3(0.0);
    if (hasTextureEmission) {
        emission = pow(texture(texture_emission1, uv).rgb, vec3(2.2));
    } else {
        emission = material.emission;
    }
    emission *= material.emissionStrength;
    
    // Final color
    vec3 color = ambient + Lo + emission;
    
    // Tone mapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    FragColor = vec4(color, 1.0);
}