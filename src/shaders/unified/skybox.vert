#version 450

layout(set = 0, binding = 0) uniform SkyboxUBO {
    mat4 projection;
    mat4 view;
} ubo;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragTexCoord;

void main() {
    fragTexCoord = inPosition;
    // Remove translation from view matrix
    mat4 rotView = mat4(mat3(ubo.view));
    vec4 pos = ubo.projection * rotView * vec4(inPosition, 1.0);
    // Set z = w for maximum depth (drawn behind everything)
    gl_Position = pos.xyww;
}
