#version 460

struct VertexData {
    float posX, posY, posZ;
    float u, v;

    float normalX, normalY, normalZ;
    float tangentX, tangentY, tangentZ;
    float bitangentX, bitangentY, bitangentZ;
};

layout(std430, binding = 0) readonly buffer Vertices{ VertexData vertices[]; }  in_vertices;
layout(binding = 1) readonly buffer Indices {int indices[]; } in_indices;
layout(binding = 2) readonly uniform UniformBuffer{ mat4 wvp;} ubo;
layout(location = 0) out vec2 texCoord;

void main() {

    int index = in_indices.indices[gl_VertexIndex];
    VertexData vertex = in_vertices.vertices[index];

    vec3 pos = vec3(vertex.posX, vertex.posY, vertex.posZ);
    gl_Position = ubo.wvp * vec4(pos, 1.0);

    texCoord = vec2(vertex.u, vertex.v);
}
