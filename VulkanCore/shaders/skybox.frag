#version 460

layout (location = 0) in vec3 Dir;
layout (location = 0) out vec4 outColor;

layout (binding = 4) uniform samplerCube skybox;



void main()
{
    outColor = texture(skybox, Dir);
}
