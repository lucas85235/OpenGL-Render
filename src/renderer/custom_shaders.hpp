#ifndef CUSTOM_SHADERS_HPP
#define CUSTOM_SHADERS_HPP

namespace CustomShaders {

const char* SkyboxVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main() {
    TexCoords = aPos;
    // Remove a translação da matriz de visão (o céu não se move, só gira)
    vec4 pos = projection * mat4(mat3(view)) * vec4(aPos, 1.0);
    
    // Truque de profundidade: Z = W. Após a divisão perspectiva, Z vira 1.0 (profundidade máxima)
    gl_Position = pos.xyww;
}
)";

const char* SkyboxFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main() {
    vec3 envColor = texture(skybox, TexCoords).rgb;
    
    // Opcional: Aplicar tonemapping e gamma correction para ver melhor o HDR
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2)); 
    
    FragColor = vec4(envColor, 1.0);
}
)";

const char* IrradianceConvolutionFragment = R"(
#version 330 core
out vec4 FragColor;
in vec3 localPos;

uniform samplerCube environmentMap;

const float PI = 3.14159265359;

void main() {		
    // O vetor do mundo age como normal da tangente da superfície
    vec3 N = normalize(localPos);
    vec3 irradiance = vec3(0.0);   
    
    // Espaço tangente baseada na normal
    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up         = normalize(cross(N, right));
       
    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    
    // Convolução: integral sobre o hemisfério
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            // Esférico para Cartesiano (no espaço tangente)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // Espaço tangente para Espaço mundo
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N; 

            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));
    
    FragColor = vec4(irradiance, 1.0);
}
)";

const char* PrefilterFragment = R"(
#version 330 core
out vec4 FragColor;
in vec3 localPos;

uniform samplerCube environmentMap;
uniform float roughness;

const float PI = 3.14159265359;

// Distribuição GGX (Normal Distribution Function)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / denom;
}

// Sequência de Hammersley para Importance Sampling (Low Discrepancy Sequence)
float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N) {
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness*roughness;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
    
    // Esférico para Cartesiano
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    
    // Tangente para Mundo
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

void main() {		
    vec3 N = normalize(localPos);    
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u; // Qualidade da amostragem
    float totalWeight = 0.0;   
    vec3 prefilteredColor = vec3(0.0);     

    for(uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H  = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0) {
            float D = DistributionGGX(N, H, roughness);
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001; 

            float resolution = 512.0; // Resolução do cubemap original
            float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 
            
            prefilteredColor += textureLod(environmentMap, L, mipLevel).rgb * NdotL;
            totalWeight      += NdotL;
        }
    }
    prefilteredColor = prefilteredColor / totalWeight;

    FragColor = vec4(prefilteredColor, 1.0);
}
)";

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

// --- LIGHT STRUCTURES ---
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

// --- UNIFORMS ---
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

// IBL Maps
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D   brdfLUT;
uniform bool        useIBL;

const float PI = 3.14159265359;

// --- SAFE PBR FUNCTIONS ---

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
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / max(denom, 0.0001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0001);
    float NdotL = max(dot(N, L), 0.0001);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// --- LIGHT CALCULATION ---
vec3 CalcPBRLight(vec3 L, vec3 V, vec3 N, vec3 F0, vec3 albedo, float metallic, float roughness, vec3 radiance) {
    vec3 H = normalize(V + L);
    
    float NDF = DistributionGGX(N, H, roughness);   
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
       
    vec3 numerator    = NDF * G * F; 
    
    float NdotV = max(dot(N, V), 0.0001);
    float NdotL = max(dot(N, L), 0.0001);
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;     

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main() {
    // 1. Material Properties
    vec3 albedo = material.albedo;
    if (hasTextureDiffuse) {
        vec4 albedoTex = texture(texture_diffuse1, TexCoords / 0.5);
        albedo = pow(albedoTex.rgb, vec3(2.2)); // Convert to linear space
    }
    // FragColor = vec4(albedo, 1.0);
    FragColor = vec4(vec3(TexCoords.x, TexCoords.y, 0.0f), 1.0);
    return;

    float metallic = material.metallic;
    if (hasTextureMetallic) metallic = texture(texture_metallic1, TexCoords).r;
    
    float roughness = material.roughness;
    if (hasTextureRoughness) roughness = texture(texture_roughness1, TexCoords).r;
    
    // Clamp roughness to prevent artifacts
    roughness = clamp(roughness, 0.04, 1.0);

    float ao = material.ao;
    if (hasTextureAO) ao = texture(texture_ao1, TexCoords).r;

    // 2. Normal / Geometry Data
    vec3 N = normalize(Normal);
    if (hasTextureNormal) {
        vec3 normalMap = texture(texture_normal1, TexCoords).rgb;
        normalMap = normalMap * 2.0 - 1.0;
        N = normalize(TBN * normalMap);
    }
    vec3 V = normalize(viewPos - FragPos);
    
    // FIX: Ensure V and N are properly oriented
    float NdotV = dot(N, V);
    if (NdotV < 0.0) {
        N = -N; // Flip normal if facing away
        NdotV = -NdotV;
    }
    NdotV = max(NdotV, 0.0001);

    // 3. Fresnel Base Reflectivity
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // --- DIRECT LIGHTING ---
    vec3 Lo = vec3(0.0);

    // Directional Light
    {
        vec3 L = normalize(-dirLight.direction);
        vec3 radiance = dirLight.color * dirLight.intensity;
        Lo += CalcPBRLight(L, V, N, F0, albedo, metallic, roughness, radiance);
    }

    // Point Lights
    for(int i = 0; i < numPointLights; ++i) {
        vec3 L = normalize(pointLights[i].position - FragPos);
        float distance = length(pointLights[i].position - FragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = pointLights[i].color * pointLights[i].intensity * attenuation;

        Lo += CalcPBRLight(L, V, N, F0, albedo, metallic, roughness, radiance);
    }

    // --- INDIRECT LIGHTING (IBL) ---
    vec3 ambient = vec3(0.0);
    
    if (useIBL) {
        // FIX: Use proper NdotV for Fresnel calculation
        vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
        
        vec3 kS = F;
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - metallic;
        
        // Diffuse IBL
        vec3 irradiance = texture(irradianceMap, N).rgb;
        vec3 diffuse = kD * irradiance * albedo;
        
        // Specular IBL
        // FIX: Calculate reflection vector properly
        vec3 R = reflect(-V, N);
        
        // FIX: Use proper mip level calculation
        const float MAX_REFLECTION_LOD = 4.0;
        float lod = roughness * MAX_REFLECTION_LOD;
        vec3 prefilteredColor = textureLod(prefilterMap, R, lod).rgb;
        
        // FIX: Sample BRDF LUT with correct coordinates (NdotV, roughness)
        vec2 envBRDF = texture(brdfLUT, vec2(NdotV, roughness)).rg;
        
        // FIX: Correct specular IBL calculation
        vec3 specular = prefilteredColor * (F0 * envBRDF.x + envBRDF.y);

        ambient = (diffuse + specular) * ao;
    } else {
        // Fallback ambient
        ambient = vec3(0.03) * albedo * ao;
    }

    // --- EMISSION ---
    vec3 emission = vec3(0.0);
    if (hasTextureEmission) {
        emission = texture(texture_emission1, TexCoords).rgb;
        emission = pow(emission, vec3(2.2)); // Convert to linear space
    } else {
        emission = material.emission;
    }
    emission *= material.emissionStrength;

    // --- COMPOSITION ---
    vec3 color = ambient + Lo + emission;

    // Tone Mapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma Correction
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}
)";

} // namespace CustomShaders

#endif // CUSTOM_SHADERS_HPP
