//====================================================================
// Skybox Shader - Unified Format
//====================================================================

#pragma shader_type(graphics)

//--------------------------------------------------------------------
// COMMON
//--------------------------------------------------------------------
#pragma begin(common)

// No shared functions needed for simple skybox

#pragma end(common)

//--------------------------------------------------------------------
// RESOURCES
//--------------------------------------------------------------------
#pragma begin(resources)

@binding(0) uniform SceneUBO {
    mat4 view;
    mat4 projection;
} ubo;

@binding(1) uniform samplerCube skyboxCubemap;

#pragma end(resources)

//--------------------------------------------------------------------
// VERTEX STAGE
//--------------------------------------------------------------------
#pragma begin(vertex)

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragTexCoord;

void main() {
    fragTexCoord = inPosition;
    
    // Remove translation from view matrix
    mat4 viewNoTranslation = mat4(mat3(ubo.view));
    vec4 pos = ubo.projection * viewNoTranslation * vec4(inPosition, 1.0);
    
    // Set z = w so depth is always 1.0 (far plane)
    gl_Position = pos.xyww;
}

#pragma end(vertex)

//--------------------------------------------------------------------
// FRAGMENT STAGE
//--------------------------------------------------------------------
#pragma begin(fragment)

layout(location = 0) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 color = texture(skyboxCubemap, fragTexCoord).rgb;
    
    // HDR tone mapping
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    outColor = vec4(color, 1.0);
}

#pragma end(fragment)
