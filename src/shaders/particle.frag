#version 450 core

in vec2 TexCoords;
in vec4 ParticleColor;

out vec4 FragColor;

uniform sampler2D Texture; // Optional particle texture
uniform bool hasTexture;

void main()
{
    vec4 texColor = vec4(1.0);
    // Ideally we pass an uniform or use a trick to know if 'Texture' is bound. Let's assume an unbound texture returns white, or we use a macro.
    // For now we just sample standard texture
    texColor = texture(Texture, TexCoords);
    
    FragColor = texColor * ParticleColor;
}
