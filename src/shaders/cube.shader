//====================================================================
// Cube PBR Shader - Unified Format
// Simple PBR shader for Vulkan cube demo
//====================================================================

#pragma shader_type(graphics)

//--------------------------------------------------------------------
// COMMON
//--------------------------------------------------------------------
#pragma begin(common)

const float PI = 3.14159265359;

struct SceneData {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 _materialColor;
    vec4 _materialProps;
    vec4 lightDir;
    vec4 lightColor;
    vec4 viewPos;
};

struct MaterialData {
    vec4 materialColor;
    vec4 materialProps;
};

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
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
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

#pragma end(common)

//--------------------------------------------------------------------
// RESOURCES
//--------------------------------------------------------------------
#pragma begin(resources)

@binding(0) uniform SceneUBO {
    SceneData scene;
} ubo;

@binding(1) uniform sampler2D texDiffuse;

@push_constant uniform PushConstants {
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
    vec4 worldPos = ubo.scene.model * vec4(inPosition, 1.0);
    fragPos = worldPos.xyz;
    
    mat3 normalMatrix = transpose(inverse(mat3(ubo.scene.model)));
    fragNormal = normalMatrix * inNormal;
    fragTexCoord = inTexCoord;
    
    gl_Position = ubo.scene.proj * ubo.scene.view * worldPos;
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
    vec3 albedo = pc.material.materialColor.rgb;
    float metallic = pc.material.materialColor.a;
    float roughness = max(pc.material.materialProps.x, 0.04);
    float ao = pc.material.materialProps.y;
    float hasTexture = pc.material.materialProps.w;
    
    if (hasTexture > 0.5) {
        vec4 texColor = texture(texDiffuse, fragTexCoord);
        albedo = pow(texColor.rgb, vec3(2.2));
    }
    
    vec3 lightDir = normalize(ubo.scene.lightDir.xyz);
    float lightIntensity = ubo.scene.lightDir.w;
    vec3 lightColor = ubo.scene.lightColor.rgb;
    vec3 viewPos = ubo.scene.viewPos.xyz;
    
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(viewPos - fragPos);
    vec3 L = normalize(-lightDir);
    vec3 H = normalize(V + L);
    
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    
    float NdotL = max(dot(N, L), 0.0);
    vec3 radiance = lightColor * lightIntensity;
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
    
    vec3 ambient = vec3(0.1) * albedo * ao;
    vec3 color = ambient + Lo;
    
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));
    
    outColor = vec4(color, 1.0);
}

#pragma end(fragment)
