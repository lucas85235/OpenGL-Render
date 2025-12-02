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
    
    vec2 uv = TexCoords;
    vec3 albedo = material.albedo;

    if (hasTextureDiffuse) {
        vec4 albedoTex = texture(texture_diffuse1, uv / 0.5);
        albedo = pow(albedoTex.rgb, vec3(2.2)); // Convert to linear space
    }

    float metallic = material.metallic;
    if (hasTextureMetallic) metallic = texture(texture_metallic1, uv).r;
    
    float roughness = material.roughness;
    if (hasTextureRoughness) roughness = texture(texture_roughness1, uv).r;
    
    // Clamp roughness to prevent artifacts
    roughness = clamp(roughness, 0.04, 1.0);

    float ao = material.ao;
    if (hasTextureAO) ao = texture(texture_ao1, uv).r;

    // 2. Normal / Geometry Data
    vec3 N = normalize(Normal);
    if (hasTextureNormal) {
        vec3 normalMap = texture(texture_normal1, uv).rgb;
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
        emission = texture(texture_emission1, uv).rgb;
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