//====================================================================
// Picking Shader - Unified Format
// Renders each entity with a flat color packed from its uint32 entity
// id. RGBA8 target: r = id & 0xFF, g = (id >> 8) & 0xFF, b = (id >> 16)
// & 0xFF, a = (id >> 24) & 0xFF. CPU encodes into u_entity_color.
// OpenGL only (non-block uniforms).
//====================================================================

#pragma shader_type(graphics)
#pragma opengl_only

//--------------------------------------------------------------------
// COMMON
//--------------------------------------------------------------------
#pragma begin(common)

// No shared helpers.

#pragma end(common)

//--------------------------------------------------------------------
// RESOURCES
//--------------------------------------------------------------------
#pragma begin(resources)

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform vec4 u_entity_color;

#pragma end(resources)

//--------------------------------------------------------------------
// VERTEX STAGE
//--------------------------------------------------------------------
#pragma begin(vertex)

layout(location = 0) in vec3 aPos;

void main() {
    gl_Position = u_projection * u_view * u_model * vec4(aPos, 1.0);
}

#pragma end(vertex)

//--------------------------------------------------------------------
// FRAGMENT STAGE
//--------------------------------------------------------------------
#pragma begin(fragment)

layout(location = 0) out vec4 outColor;

void main() {
    outColor = u_entity_color;
}

#pragma end(fragment)
