#version 450

// Scene uniforms (per-frame, binding 0)
layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 projection;
    vec4 cameraPos;      // xyz = position
    vec4 lightDir;       // xyz = direction, w = intensity
    vec4 lightColor;     // xyz = color
} scene;

// Per-object data (push constants)
layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 materialColor;  // rgb = albedo, a = metallic
    vec4 materialProps;  // r = roughness, g = ao, b = emissionStrength, a = flags
} pc;

// Vertex inputs
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

// Outputs to fragment shader
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out mat3 fragTBN;

void main() {
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    fragPos = worldPos.xyz;
    
    mat3 normalMatrix = transpose(inverse(mat3(pc.model)));
    fragNormal = normalMatrix * inNormal;
    
    vec3 T = normalize(normalMatrix * inTangent);
    vec3 B = normalize(normalMatrix * inBitangent);
    vec3 N = normalize(normalMatrix * inNormal);
    fragTBN = mat3(T, B, N);
    
    fragTexCoord = inTexCoord;
    
    gl_Position = scene.projection * scene.view * worldPos;
}
