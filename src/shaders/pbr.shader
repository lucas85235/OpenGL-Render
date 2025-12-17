//====================================================================
// PBR Shader - Unified Format
// Single file containing vertex and fragment stages
//====================================================================

#pragma shader_type(graphics)

//--------------------------------------------------------------------
// COMMON - Shared code across all stages
//--------------------------------------------------------------------
#pragma begin(common)

#define MAX_POINT_LIGHTS 4
const float PI = 3.14159265359;

struct PointLight {
    vec4 positionIntensity;  // xyz = position, w = intensity
    vec4 colorRadius;        // rgb = color, a = radius
};

struct SceneData {
    mat4 view;
    mat4 proj;
    vec4 lightDir;           // xyz = direction, w = intensity
    vec4 lightColor;         // rgb = color
    vec4 viewPos;            // xyz = camera position
    PointLight pointLights[MAX_POINT_LIGHTS];
    int numPointLights;
    int _pad[3];
};

struct MaterialData {
    vec4 colorMetallic;      // rgb = albedo, a = metallic
    vec4 props;              // r = roughness, g = ao, b = emissionStrength, a = flags
};

// Material flags
const uint FLAG_HAS_DIFFUSE   = 1u;
const uint FLAG_HAS_NORMAL    = 2u;
const uint FLAG_HAS_METALLIC  = 4u;
const uint FLAG_HAS_ROUGHNESS = 8u;
const uint FLAG_HAS_AO        = 16u;
const uint FLAG_HAS_EMISSION  = 32u;
const uint FLAG_USE_IBL       = 64u;

bool hasFlag(uint flags, uint flag) {
    return (flags & flag) != 0u;
}

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
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
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

#pragma end(common)

//--------------------------------------------------------------------
// RESOURCES - Uniforms and samplers (API-agnostic bindings)
//--------------------------------------------------------------------
#pragma begin(resources)

@binding(0) uniform SceneData uScene;

@binding(1) uniform sampler2D texDiffuse;
@binding(2) uniform sampler2D texNormal;
@binding(3) uniform sampler2D texMetallic;
@binding(4) uniform sampler2D texRoughness;
@binding(5) uniform sampler2D texAO;
@binding(6) uniform sampler2D texEmission;

@push_constant uniform PushConstants {
    mat4 model;
    MaterialData material;
} pc;

#pragma end(resources)

//--------------------------------------------------------------------
// VERTEX STAGE
//--------------------------------------------------------------------
#pragma begin(vertex)

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

void main() {
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    fragPos = worldPos.xyz;
    
    mat3 normalMatrix = transpose(inverse(mat3(pc.model)));
    fragNormal = normalMatrix * inNormal;
    fragTexCoord = inTexCoord;
    
    gl_Position = uScene.proj * uScene.view * worldPos;
}

#pragma end(vertex)

//--------------------------------------------------------------------
// FRAGMENT STAGE
//--------------------------------------------------------------------
#pragma begin(fragment)

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    uint flags = uint(pc.material.props.a);
    
    // Material properties
    vec3 albedo = pc.material.colorMetallic.rgb;
    float metallic = pc.material.colorMetallic.a;
    float roughness = pc.material.props.r;
    float ao = pc.material.props.g;
    float emissionStrength = pc.material.props.b;
    
    // Sample textures if available
    if (hasFlag(flags, FLAG_HAS_DIFFUSE)) {
        vec4 texColor = texture(texDiffuse, fragTexCoord);
        albedo = pow(texColor.rgb, vec3(2.2));
    }
    if (hasFlag(flags, FLAG_HAS_METALLIC)) {
        metallic = texture(texMetallic, fragTexCoord).r;
    }
    if (hasFlag(flags, FLAG_HAS_ROUGHNESS)) {
        roughness = texture(texRoughness, fragTexCoord).r;
    }
    if (hasFlag(flags, FLAG_HAS_AO)) {
        ao = texture(texAO, fragTexCoord).r;
    }
    
    roughness = clamp(roughness, 0.04, 1.0);
    
    // Normals
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(uScene.viewPos.xyz - fragPos);
    
    float NdotV = dot(N, V);
    if (NdotV < 0.0) {
        N = -N;
        NdotV = -NdotV;
    }
    NdotV = max(NdotV, 0.0001);
    
    // Fresnel F0
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    
    // Direct lighting
    vec3 Lo = vec3(0.0);
    
    // Directional light
    vec3 L = normalize(-uScene.lightDir.xyz);
    vec3 radiance = uScene.lightColor.rgb * uScene.lightDir.w;
    Lo += CalcPBRLight(L, V, N, F0, albedo, metallic, roughness, radiance);
    
    // Point lights
    for (int i = 0; i < uScene.numPointLights && i < MAX_POINT_LIGHTS; i++) {
        vec3 lightPos = uScene.pointLights[i].positionIntensity.xyz;
        float lightIntensity = uScene.pointLights[i].positionIntensity.w;
        vec3 lightColor = uScene.pointLights[i].colorRadius.rgb;
        float lightRadius = uScene.pointLights[i].colorRadius.a;
        
        vec3 lightVec = lightPos - fragPos;
        float distance = length(lightVec);
        vec3 Li = normalize(lightVec);
        
        float attenuation = 1.0 / (distance * distance + 1.0);
        float falloff = clamp(1.0 - pow(distance / max(lightRadius, 0.001), 4.0), 0.0, 1.0);
        falloff = falloff * falloff;
        
        vec3 pointRadiance = lightColor * lightIntensity * attenuation * falloff;
        Lo += CalcPBRLight(Li, V, N, F0, albedo, metallic, roughness, pointRadiance);
    }
    
    // Ambient
    vec3 ambient = vec3(0.03) * albedo * ao;
    
    // Emission
    vec3 emission = vec3(0.0);
    if (hasFlag(flags, FLAG_HAS_EMISSION)) {
        emission = pow(texture(texEmission, fragTexCoord).rgb, vec3(2.2));
        emission *= emissionStrength;
    }
    
    // Final composition
    vec3 color = ambient + Lo + emission;
    
    // Tone mapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    outColor = vec4(color, 1.0);
}

#pragma end(fragment)
