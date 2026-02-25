highp vec4 Output;

highp vec4 char1(vec2 p, int c, sampler2D image)
{
    float code_i = float(c % 16);
    float code_j = float(c / 16);

    p += vec2(code_i, code_j);
    p /= 16.;
    return textureGrad(image, p, vec2(0.0),vec2(0.0));
}

vec4 setMappingValue(float maxSoftEdge, float minSoftEdge, sampler2D textureMapping, int atlasSize, vec2 fragCoord, sampler2D image, float fontSize, int stringLength) {

    Output = vec4(0.0);

    vec2 scale = vec2(1./ fontSize);

    vec2 cellSize = 16. * scale;
    
    vec2 pos = fragCoord * cellSize;
    vec2 pos_integer = floor(pos);

    int cellIndex = int(pos_integer.x + cellSize.x * pos_integer.y);

    if(cellIndex >= stringLength)
        return Output.xxxx;
    
    int i = int(cellIndex) % atlasSize;
    int j = int(cellIndex) / atlasSize;

    vec4 outText = texture(textureMapping, vec2(i, j)/ float(atlasSize - 1));

    // TODO: spacing between letters
    pos -= pos_integer;

    //TODO: parameterize, improve this to make possible the character spacing
    // pos -= pos_integer - vec2(0.5,0.25);
    // pos *= vec2(0.5,0.75);  

    Output += char1(pos, int(outText.x * 255.), image);
    
    Output *= 2.;
    
    Output = smoothstep(minSoftEdge,maxSoftEdge,Output);
    return vec4(Output.xxxx);
}
