#pragma shader_type(graphics)

#pragma begin(vertex)

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec4 ParticleColor;

uniform mat4 view;
uniform mat4 projection;

uniform sampler2D Tex;
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
    int id = gl_InstanceID;
    
    float u = (float(id % TextureWidth) + 0.5) / float(TextureWidth);
    float v = (float(id / TextureWidth) + 0.5) / float(TextureWidth);
    
    vec4 texPos = texture(Tex, vec2(u, v));
    
    vec3 instancePos = Position + texPos.xyz * Radius;
    
    vec3 toCamera = normalize(CameraPosition - instancePos);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), toCamera));
    vec3 up = cross(toCamera, right);

    vec3 vertexOffset = aPos;
    vertexOffset.x *= Size.x;
    vertexOffset.y *= Size.y;

    vec3 worldPos = instancePos + (right * vertexOffset.x) + (up * vertexOffset.y);
    
    gl_Position = projection * view * vec4(worldPos, 1.0);
    
    TexCoords = aTexCoords;
    ParticleColor = vec4(Color, texPos.w > 0.0 ? texPos.w : 1.0);
}
#pragma end(vertex)

#pragma begin(fragment)

in vec2 TexCoords;
in vec4 ParticleColor;

out vec4 FragColor;

uniform sampler2D Texture;
uniform bool hasTexture;

void main()
{
    vec4 texColor = vec4(1.0);
    if (hasTexture) { 
       texColor = texture(Texture, TexCoords);
    }
    
    FragColor = texColor * ParticleColor;
}
#pragma end(fragment)
