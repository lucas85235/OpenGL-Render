vec4 getPixelValue(int index, int textureWidth) {

    // Calculate x and y coordinates from index
    int x = index % textureWidth;
    int y = index / textureWidth;

    // Calculate normalized texture coordinates
    float u = (float(x) + 0.5) / float(textureWidth);
    float v = (float(y) + 0.5) / float(textureWidth);

    vec4 texValue = texture(materialParams_Tex, vec2(u, v));
    return texValue;
}
