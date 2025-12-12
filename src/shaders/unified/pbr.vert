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

// Vertex inputs (must match Renderer vertex layout)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

// Outputs to fragment shader
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

void main() {
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    fragPos = worldPos.xyz;
    
    mat3 normalMatrix = transpose(inverse(mat3(pc.model)));
    fragNormal = normalMatrix * inNormal;
    
    fragTexCoord = inTexCoord;
    
    gl_Position = ubo.proj * ubo.view * worldPos;
}
