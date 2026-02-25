vec3 sdgCircle( in vec2 p, in float r ) 
{
    float d = length(p);
    return vec3( d-r, p/d );
}

vec4 getEffectColor(vec2 uv, float deltaTime){
    
    vec2 pos = vec2(0.0);
    vec4 col = vec4(sin(deltaTime * 1.24)*0.5+0.5,sin(deltaTime * 2.56)*0.5+0.5,sin(deltaTime * 4.2)*0.5+0.5,sin(deltaTime * 1.23)*0.5+0.5);
    return col;
}