//====================================================================
// Equirectangular to Cubemap Shader - Unified Format
// Converts HDR equirectangular maps to cubemap faces
//====================================================================

#pragma shader_type(graphics)

//--------------------------------------------------------------------
// COMMON
//--------------------------------------------------------------------
#pragma begin(common)

const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

#pragma end(common)

//--------------------------------------------------------------------
// RESOURCES
//--------------------------------------------------------------------
#pragma begin(resources)

@binding(0) uniform TransformUBO {
    mat4 projection;
    mat4 view;
} ubo;

@binding(1) uniform sampler2D equirectangularMap;

#pragma end(resources)

//--------------------------------------------------------------------
// VERTEX STAGE
//--------------------------------------------------------------------
#pragma begin(vertex)

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 localPos;

void main() {
    localPos = inPosition;
    gl_Position = ubo.projection * ubo.view * vec4(inPosition, 1.0);
}

#pragma end(vertex)

//--------------------------------------------------------------------
// FRAGMENT STAGE
//--------------------------------------------------------------------
#pragma begin(fragment)

layout(location = 0) in vec3 localPos;

layout(location = 0) out vec4 fragColor;

void main() {
    vec3 N = normalize(localPos);
    vec2 uv = SampleSphericalMap(N);
    vec3 color = texture(equirectangularMap, uv).rgb;
    fragColor = vec4(color, 1.0);
}

#pragma end(fragment)
