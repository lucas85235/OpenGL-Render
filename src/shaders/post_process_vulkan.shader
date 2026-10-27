//====================================================================
// Post-Process Shader - Unified Format (Vulkan)
// Mirrors post_process_opengl.shader using push constants + descriptor set.
// Multiple effects controlled by pc.effectType.
//====================================================================

#pragma shader_type(graphics)

//--------------------------------------------------------------------
// COMMON
//--------------------------------------------------------------------
#pragma begin(common)

// Effect types (must match PostProcessEffectType enum)
// 0 = None, 1 = Grayscale, 2 = Sepia, 3 = Invert
// 4 = Vignette, 5 = ChromaticAberration, 6 = Sharpen, 7 = EdgeDetection

#pragma end(common)

//--------------------------------------------------------------------
// RESOURCES
//--------------------------------------------------------------------
#pragma begin(resources)

@binding(0) uniform sampler2D screenTexture;

@push_constant uniform PushConstants {
    int   effectType;
    float intensity;
    float vignetteRadius;
    float vignetteStrength;
    float chromaticOffset;
    float sharpenStrength;
    vec2  resolution;
} pc;

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

vec3 applyGrayscale(vec3 color, float intensity) {
    float gray = dot(color, vec3(0.299, 0.587, 0.114));
    return mix(color, vec3(gray), intensity);
}

vec3 applySepia(vec3 color, float intensity) {
    vec3 sepia;
    sepia.r = dot(color, vec3(0.393, 0.769, 0.189));
    sepia.g = dot(color, vec3(0.349, 0.686, 0.168));
    sepia.b = dot(color, vec3(0.272, 0.534, 0.131));
    return mix(color, sepia, intensity);
}

vec3 applyInvert(vec3 color, float intensity) {
    return mix(color, vec3(1.0) - color, intensity);
}

vec3 applyVignette(vec3 color, vec2 uv, float radius, float strength, float intensity) {
    vec2 center = uv - 0.5;
    float dist = length(center);
    float vignette = 1.0 - smoothstep(radius - 0.1, radius + 0.2, dist);
    vignette = mix(1.0, vignette, strength);
    return mix(color, color * vignette, intensity);
}

vec3 applyChromaticAberration(vec2 uv, float offset, float intensity) {
    float r = texture(screenTexture, uv + vec2(offset, 0.0)).r;
    float g = texture(screenTexture, uv).g;
    float b = texture(screenTexture, uv - vec2(offset, 0.0)).b;
    vec3 original = texture(screenTexture, uv).rgb;
    return mix(original, vec3(r, g, b), intensity);
}

vec3 applySharpen(vec2 uv, float strength, float intensity) {
    vec2 texelSize = 1.0 / textureSize(screenTexture, 0);
    vec3 center = texture(screenTexture, uv).rgb;
    vec3 top = texture(screenTexture, uv + vec2(0.0, texelSize.y)).rgb;
    vec3 bottom = texture(screenTexture, uv - vec2(0.0, texelSize.y)).rgb;
    vec3 left = texture(screenTexture, uv - vec2(texelSize.x, 0.0)).rgb;
    vec3 right = texture(screenTexture, uv + vec2(texelSize.x, 0.0)).rgb;

    vec3 sharpened = center * (1.0 + 4.0 * strength)
                   - (top + bottom + left + right) * strength;
    return mix(center, sharpened, intensity);
}

vec3 applyEdgeDetection(vec2 uv, float intensity) {
    vec2 texelSize = 1.0 / textureSize(screenTexture, 0);

    // Sobel operator
    float kernelX[9] = float[](-1, 0, 1, -2, 0, 2, -1, 0, 1);
    float kernelY[9] = float[](-1, -2, -1, 0, 0, 0, 1, 2, 1);

    vec3 sumX = vec3(0.0);
    vec3 sumY = vec3(0.0);

    int idx = 0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            vec3 s = texture(screenTexture, uv + offset).rgb;
            sumX += s * kernelX[idx];
            sumY += s * kernelY[idx];
            idx++;
        }
    }

    vec3 edge = sqrt(sumX * sumX + sumY * sumY);
    vec3 original = texture(screenTexture, uv).rgb;
    return mix(original, edge, intensity);
}

void main() {
    vec3 color = texture(screenTexture, fragTexCoord).rgb;

    if (pc.effectType == 1) {
        color = applyGrayscale(color, pc.intensity);
    } else if (pc.effectType == 2) {
        color = applySepia(color, pc.intensity);
    } else if (pc.effectType == 3) {
        color = applyInvert(color, pc.intensity);
    } else if (pc.effectType == 4) {
        color = applyVignette(color, fragTexCoord, pc.vignetteRadius, pc.vignetteStrength, pc.intensity);
    } else if (pc.effectType == 5) {
        color = applyChromaticAberration(fragTexCoord, pc.chromaticOffset, pc.intensity);
    } else if (pc.effectType == 6) {
        color = applySharpen(fragTexCoord, pc.sharpenStrength, pc.intensity);
    } else if (pc.effectType == 7) {
        color = applyEdgeDetection(fragTexCoord, pc.intensity);
    }
    // effectType == 0: pass-through

    outColor = vec4(color, 1.0);
}

#pragma end(fragment)
