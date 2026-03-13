#pragma shader_type(graphics)

#pragma begin(vertex)

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec4 ParticleColor;

uniform mat4 model;
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
    // Calculate 2D ID from 1D gl_InstanceID using TextureWidth
    int id = gl_InstanceID;
    
    // Convert ID to 2D coordinates
    // We sample texture at exact texel centers
    float u = (float(id % TextureWidth) + 0.5) / float(TextureWidth);
    float v = (float(id / TextureWidth) + 0.5) / float(TextureWidth);
    
    // Sample position offset
    vec4 texPos = texture(Tex, vec2(u, v));
    
    // Base position for this instance
    vec3 instancePos = Position + texPos.xyz * Radius;
    
    // Viewport billboarding
    // Remove rotation from view matrix for billboarding
    vec3 wCameraPos = CameraPosition;
    vec3 toCamera = normalize(wCameraPos - instancePos);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), toCamera));
    vec3 up = cross(toCamera, right);

    vec3 vertexOffset = aPos;
    vertexOffset.x *= Size.x;
    vertexOffset.y *= Size.y;

    vec3 worldPos = instancePos + (right * vertexOffset.x) + (up * vertexOffset.y);
    
    gl_Position = projection * view * model * vec4(worldPos, 1.0);
    
    TexCoords = aTexCoords;
    ParticleColor = vec4(Color, texPos.w > 0.0 ? texPos.w : 1.0);
}
#pragma end(vertex)

#pragma begin(fragment)

in vec2 TexCoords;
in vec4 ParticleColor;

out vec4 FragColor;

uniform sampler2D Texture;

void main()
{
    vec4 texColor = texture(Texture, TexCoords);
    if(texColor.a < 0.1) discard;
    
    FragColor = texColor * ParticleColor;
}
#pragma end(fragment)
