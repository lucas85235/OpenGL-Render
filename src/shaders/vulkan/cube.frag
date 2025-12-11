#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 materialColor;  // albedo.rgb + metallic
    vec4 materialProps;  // roughness, ao, emissionStrength, unused
    vec4 lightDir;       // direction.xyz + intensity
    vec4 lightColor;     // color.rgb + unused
    vec4 viewPos;        // position.xyz + unused
} ubo;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

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

void main() {
    // Material from UBO
    vec3 albedo = ubo.materialColor.rgb;
    float metallic = ubo.materialColor.a;
    float roughness = max(ubo.materialProps.x, 0.04);
    float ao = ubo.materialProps.y;
    
    // Light from UBO
    vec3 lightDir = normalize(ubo.lightDir.xyz);
    float lightIntensity = ubo.lightDir.w;
    vec3 lightColor = ubo.lightColor.rgb;
    vec3 viewPos = ubo.viewPos.xyz;
    
    // Geometry
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(viewPos - fragPos);
    vec3 L = normalize(-lightDir);
    vec3 H = normalize(V + L);
    
    // Fresnel
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    // Energy conservation
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    
    // Direct lighting
    float NdotL = max(dot(N, L), 0.0);
    vec3 radiance = lightColor * lightIntensity;
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
    
    // Ambient
    vec3 ambient = vec3(0.03) * albedo * ao;
    
    // Final color
    vec3 color = ambient + Lo;
    
    // Tone mapping
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    outColor = vec4(color, 1.0);
}
