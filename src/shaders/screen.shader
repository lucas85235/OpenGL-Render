//====================================================================
// Screen Shader - Unified Format
// Fullscreen quad for post-processing
//====================================================================

#pragma shader_type(graphics)

//--------------------------------------------------------------------
// COMMON
//--------------------------------------------------------------------
#pragma begin(common)

// No common code needed

#pragma end(common)

//--------------------------------------------------------------------
// RESOURCES
//--------------------------------------------------------------------
#pragma begin(resources)

@binding(0) uniform sampler2D screenTexture;

#pragma end(resources)

//--------------------------------------------------------------------
// VERTEX STAGE
//--------------------------------------------------------------------
#pragma begin(vertex)

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragTexCoord = inTexCoord;
}

#pragma end(vertex)

//--------------------------------------------------------------------
// FRAGMENT STAGE
//--------------------------------------------------------------------
#pragma begin(fragment)

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 color = texture(screenTexture, fragTexCoord).rgb;
    outColor = vec4(color, 1.0);
}

#pragma end(fragment)
