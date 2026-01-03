#version 460

layout (location = 0) out vec3 Dir;
layout (binding = 2) readonly uniform UniformBuffer{ mat4 vp;} ubo;

const vec3 pos[8] = vec3[](
    vec3(-1.0, -1.0,  1.0),     // 0
    vec3( 1.0, -1.0,  1.0),     // 1
    vec3( 1.0,  1.0,  1.0),     // 2
    vec3(-1.0,  1.0,  1.0),     // 3

    vec3(-1.0, -1.0, -1.0),     // 4
    vec3( 1.0, -1.0, -1.0),     // 5
    vec3( 1.0,  1.0, -1.0),     // 6
    vec3(-1.0,  1.0, -1.0)      // 7
);

const int indices[36] = int[](
    0, 1, 2, 2, 3, 0,       // Front face
    1, 5, 6, 6, 2, 1,       // Right face
    5, 4, 7, 7, 6, 5,       // Back face
    4, 0, 3, 3, 7, 4,       // Left face
    3, 2, 6, 6, 7, 3,       // Top face
    4, 5, 1, 1, 0, 4        // Bottom face
);


void main()
{
    int idx = indices[gl_VertexIndex];
    vec4 worldPos = ubo.vp * vec4(pos[idx], 1.0);
    gl_Position = worldPos.xyww;   // perspective divide w = z for skybox depth trick : always at far plane

    Dir = pos[idx].xyz;
}
