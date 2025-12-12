#version 450

// Scene-wide data (updated once per frame)
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 lightDir;       // xyz = direction, w = intensity
    vec4 lightColor;     // xyz = color
    vec4 viewPos;        // xyz = camera position
} ubo;

// Per-draw data via push constants (updated per mesh)
layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 materialColor;  // rgb = albedo, a = metallic
    vec4 materialProps;  // r = roughness, g = ao, b = emissionStrength, a = flags
} pc;

// Texture samplers
layout(set = 0, binding = 1) uniform sampler2D texDiffuse;
layout(set = 0, binding = 2) uniform sampler2D texNormal;
layout(set = 0, binding = 3) uniform sampler2D texMetallic;
layout(set = 0, binding = 4) uniform sampler2D texRoughness;
layout(set = 0, binding = 5) uniform sampler2D texAO;
layout(set = 0, binding = 6) uniform sampler2D texEmission;

// IBL maps (Temporarily disabled to match RHI descriptor layout)
// layout(set = 0, binding = 7) uniform samplerCube irradianceMap;
// layout(set = 0, binding = 8) uniform samplerCube prefilterMap;
// layout(set = 0, binding = 9) uniform sampler2D brdfLUT;

// Fragment inputs
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

// Output
layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

// Material flags (packed in materialProps.a)
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

void main() {
    uint flags = uint(pc.materialProps.a);
    
    // Material properties from push constants
    vec3 albedo = pc.materialColor.rgb;
    float metallic = pc.materialColor.a;
    float roughness = pc.materialProps.r;
    float ao = pc.materialProps.g;
    float emissionStrength = pc.materialProps.b;
    
    // Sample textures if available
    if (hasFlag(flags, FLAG_HAS_DIFFUSE)) {
        vec4 texColor = texture(texDiffuse, fragTexCoord);
        albedo = pow(texColor.rgb, vec3(2.2)); // sRGB to linear
    }
    // if (hasFlag(flags, FLAG_HAS_METALLIC)) {
    //     metallic = texture(texMetallic, fragTexCoord).r;
    // }
    // if (hasFlag(flags, FLAG_HAS_ROUGHNESS)) {
    //     roughness = texture(texRoughness, fragTexCoord).r;
    // }
    // if (hasFlag(flags, FLAG_HAS_AO)) {
    //     ao = texture(texAO, fragTexCoord).r;
    // }
    
    roughness = clamp(roughness, 0.04, 1.0);
    
    // Normal calculation (TBN not available without tangent data)
    vec3 N = normalize(fragNormal);
    
    vec3 V = normalize(ubo.viewPos.xyz - fragPos);
    
    // Fix backfacing normals
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
    vec3 L = normalize(-ubo.lightDir.xyz);
    vec3 radiance = ubo.lightColor.rgb * ubo.lightDir.w;
    Lo += CalcPBRLight(L, V, N, F0, albedo, metallic, roughness, radiance);
    
    // Ambient / IBL
    vec3 ambient = vec3(0.0);
    
    if (hasFlag(flags, FLAG_USE_IBL)) {
        vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
        vec3 kS = F;
        vec3 kD = (1.0 - kS) * (1.0 - metallic);
        
        // Diffuse IBL
        // vec3 irradiance = texture(irradianceMap, N).rgb;
        vec3 irradiance = vec3(0.03); // Fallback
        vec3 diffuse = kD * irradiance * albedo;
        
        // Specular IBL
        vec3 R = reflect(-V, N);
        const float MAX_REFLECTION_LOD = 4.0;
        // vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
        vec3 prefilteredColor = vec3(0.0); // Fallback
        // vec2 envBRDF = texture(brdfLUT, vec2(NdotV, roughness)).rg;
        vec2 envBRDF = vec2(0.5, 0.5); // Fallback
        vec3 specular = prefilteredColor * (F0 * envBRDF.x + envBRDF.y);
        
        ambient = (diffuse + specular) * ao;
    } else {
        ambient = vec3(0.03) * albedo * ao;
    }
    
    // Emission
    vec3 emission = vec3(0.0);
    if (hasFlag(flags, FLAG_HAS_EMISSION)) {
        // emission = pow(texture(texEmission, fragTexCoord).rgb, vec3(2.2));
    }
    emission *= emissionStrength;
    
    // Final composition
    vec3 color = ambient + Lo + emission;
    
    // Tone mapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    outColor = vec4(color, 1.0);
}
