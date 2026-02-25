highp float3 colorTransform(highp float3 color) {
    color = mulMat3x3Float3(mat4(materialParams.colorTransform), color);
    // TODO(b/331806753): use gamut mapping rather than gamut clipping.
    return clamp(color, 0.0, 1.0);
}

// PQ EOTF based on https://www.itu.int/rec/R-REC-BT.2100.
// Converts non-linear BT2020 PQ RGB color in [0, 1] into linear RGB color in [0, 1].
highp float3 pqToLinear(highp float3 color) {
    const float3 c1 = float3(107.0 / 128.0);
    const float3 c2 = float3(2413.0 / 128.0);
    const float3 c3 = float3(2392.0 / 128.0);
    const float3 m1 = float3(1305.0 / 8192.0);
    const float3 m2 = float3(2523.0 / 32.0);
    highp float3 p = pow(color, 1.0 / m2);
    highp float3 L = pow(max(p - c1, float3(0)) / (c2 - c3 * p), 1.0 / m1);
    return L;
}

highp float3 tonemapReinhard(highp float3 color, highp float maxInput, highp float maxOutput) {
    highp float a = maxOutput / (maxInput * maxInput);
    highp float b = 1.0 / maxOutput;
    highp float maxChannel = max(max(color.r, color.g), color.b);
    return color * float3((1.0 + a * maxChannel) / (1.0 + b * maxChannel));
}

highp float3 convertColor(highp float3 color) {
    if (materialConstants_isTransferST2084) {
        color = pqToLinear(color);
    }
    if (materialConstants_isColorSpaceBT2020) {
        // Followed Chrome's implementation of Reinhard Extended.
        // https://docs.google.com/document/d/17T2ek1i2R7tXdfHCnM-i5n6__RoYe0JyMfKmTEjoGR8
        // 10000 is the maximum absolute luminance of ST2084 (PQ).
        // TODO(b/331666549): use HDR metadata from Android buffers.
        const highp float maxInputLum = 10000.0;
        const highp float maxOutputLum = 203.0;

        // Convert non-linear color into linear space, and scale the color of relative luminance
        // into absolute luminance.
        color = color * maxInputLum;
        // Divide by 203 such that (1, 1, 1) is SDR white.
        color = color / maxOutputLum;
        // Tonemap the color into [0, 1] in BT2020.
        color = tonemapReinhard(color, maxInputLum, maxOutputLum);
        color = colorTransform(color);
    }
    return color;
}