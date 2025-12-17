#version 450

uniform samplerCube skybox;

in vec3 fragTexCoord;

out vec4 outColor;

void main() {
    vec3 envColor = texture(skybox, fragTexCoord).rgb;
    
    // Tone mapping (Reinhard)
    envColor = envColor / (envColor + vec3(1.0));
    
    // Gamma correction
    envColor = pow(envColor, vec3(1.0 / 2.2));
    
    outColor = vec4(envColor, 1.0);
}
