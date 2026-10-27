//====================================================================
// Bloom Shader - Unified Format
// One shader, three fragment modes selected via u_mode:
//   0 = Prefilter  (soft-knee bright-pass on HDR scene color)
//   1 = Downsample (13-tap Call of Duty / SIGGRAPH 2014 kernel)
//   2 = Upsample   (3x3 tent filter; designed for additive blend)
// Vertex stage is a fullscreen triangle from gl_VertexID (no VBO data).
// OpenGL only — relies on gl_VertexID-driven fullscreen tri.
//====================================================================

#pragma shader_type(graphics)
#pragma opengl_only

#pragma begin(common)
#pragma end(common)

#pragma begin(resources)

@binding(0) uniform sampler2D u_source;

uniform int   u_mode;
uniform vec2  u_texelSize;
uniform float u_threshold;
uniform float u_knee;
uniform float u_filterRadius;
uniform float u_intensity;

#pragma end(resources)

#pragma begin(vertex)

layout(location = 0) out vec2 fragTexCoord;

void main() {
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    fragTexCoord = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}

#pragma end(vertex)

#pragma begin(fragment)

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

// Soft-knee bright-pass per Jorge Jimenez / Call of Duty.
vec3 prefilter(vec3 c) {
    float brightness = max(c.r, max(c.g, c.b));
    float knee = max(u_knee, 1e-4);
    float soft = brightness - u_threshold + knee;
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee);
    float contribution = max(soft, brightness - u_threshold);
    contribution /= max(brightness, 1e-4);
    return c * contribution;
}

vec3 downsample13(vec2 uv, vec2 texelSize) {
    vec3 a = texture(u_source, uv + texelSize * vec2(-2.0,  2.0)).rgb;
    vec3 b = texture(u_source, uv + texelSize * vec2( 0.0,  2.0)).rgb;
    vec3 c = texture(u_source, uv + texelSize * vec2( 2.0,  2.0)).rgb;

    vec3 d = texture(u_source, uv + texelSize * vec2(-2.0,  0.0)).rgb;
    vec3 e = texture(u_source, uv + texelSize * vec2( 0.0,  0.0)).rgb;
    vec3 f = texture(u_source, uv + texelSize * vec2( 2.0,  0.0)).rgb;

    vec3 g = texture(u_source, uv + texelSize * vec2(-2.0, -2.0)).rgb;
    vec3 h = texture(u_source, uv + texelSize * vec2( 0.0, -2.0)).rgb;
    vec3 i = texture(u_source, uv + texelSize * vec2( 2.0, -2.0)).rgb;

    vec3 j = texture(u_source, uv + texelSize * vec2(-1.0,  1.0)).rgb;
    vec3 k = texture(u_source, uv + texelSize * vec2( 1.0,  1.0)).rgb;
    vec3 l = texture(u_source, uv + texelSize * vec2(-1.0, -1.0)).rgb;
    vec3 m = texture(u_source, uv + texelSize * vec2( 1.0, -1.0)).rgb;

    vec3 result = e * 0.125;
    result += (a + c + g + i) * 0.03125;
    result += (b + d + f + h) * 0.0625;
    result += (j + k + l + m) * 0.125;
    return result;
}

// 3x3 tent filter. u_texelSize is 1 / size(source mip). Radius scales
// offsets outward; 1.0 = touching neighboring texels.
vec3 upsampleTent(vec2 uv, vec2 texelSize, float radius) {
    vec2 off = texelSize * radius;
    vec3 sum = vec3(0.0);
    sum += texture(u_source, uv + vec2(-off.x, -off.y)).rgb * 1.0;
    sum += texture(u_source, uv + vec2( 0.0,   -off.y)).rgb * 2.0;
    sum += texture(u_source, uv + vec2( off.x, -off.y)).rgb * 1.0;
    sum += texture(u_source, uv + vec2(-off.x,  0.0  )).rgb * 2.0;
    sum += texture(u_source, uv + vec2( 0.0,    0.0  )).rgb * 4.0;
    sum += texture(u_source, uv + vec2( off.x,  0.0  )).rgb * 2.0;
    sum += texture(u_source, uv + vec2(-off.x,  off.y)).rgb * 1.0;
    sum += texture(u_source, uv + vec2( 0.0,    off.y)).rgb * 2.0;
    sum += texture(u_source, uv + vec2( off.x,  off.y)).rgb * 1.0;
    return sum * (1.0 / 16.0);
}

void main() {
    vec3 color;
    if (u_mode == 0) {
        color = prefilter(texture(u_source, fragTexCoord).rgb);
    } else if (u_mode == 1) {
        color = downsample13(fragTexCoord, u_texelSize);
    } else {
        color = upsampleTent(fragTexCoord, u_texelSize, u_filterRadius) * u_intensity;
    }
    outColor = vec4(color, 1.0);
}

#pragma end(fragment)
