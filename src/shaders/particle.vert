#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec4 ParticleColor;

uniform mat4 model; // Base transform (usually from emitter entity)
uniform mat4 view;
uniform mat4 projection;

uniform sampler2D Tex; // Initial state mapping texture
uniform int TextureWidth;
uniform int Amount;

uniform vec3 CameraPosition;
uniform vec3 Velocity;
uniform vec3 Position;
uniform vec3 Color;
uniform vec2 Size;
uniform float Radius;
uniform float Angle;
uniform bool Center;

void main()
{
    // Particle index
    int id = gl_InstanceID;
    
    // Calculate UV to sample from initial state texture
    float u = float(id % TextureWidth) / float(TextureWidth);
    float v = float(id / TextureWidth) / float(TextureWidth);
    
    // Sample position offset
    vec4 texPos = texture(Tex, vec2(u, v));
    
    // Base position for this instance
    vec3 instancePos = Position + texPos.xyz * Radius;
    
    // Setup vertex offset
    vec3 vertexOffset = aPos;
    vertexOffset.x *= Size.x;
    vertexOffset.y *= Size.y;
    
    // Billboard logic: extract right and up vectors from view matrix
    // Or simpler: remove rotation from view matrix for the particle
    // Let's implement cylindrical or spherical billboarding
    vec3 wCameraPos = CameraPosition;
    vec3 toCamera = normalize(wCameraPos - instancePos);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), toCamera));
    vec3 up = cross(toCamera, right);

    vec3 worldPos = instancePos + (right * vertexOffset.x) + (up * vertexOffset.y);
    
    // Apply based on model (we might not need the full model if we use billboarding)
    // But let's apply model for any global scaling or translation
    gl_Position = projection * view * model * vec4(worldPos, 1.0);
    
    TexCoords = aTexCoords;
    ParticleColor = vec4(Color, texPos.w); // using w as some fading or alpha
}
