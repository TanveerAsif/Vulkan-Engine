#version 460

struct VertexData {
    float x;
    float y;
    float z;

    float u;
    float v;
};

layout(binding = 0) readonly buffer Vertices{ VertexData vertices[]; }  in_vertices;
layout(binding = 1) readonly uniform UniformBuffer{ mat4 wvp;} ubo;
layout(location = 0) out vec2 texCoord;

void main() {
    VertexData vertex = in_vertices.vertices[gl_VertexIndex];
    vec3 pos = vec3(vertex.x, vertex.y, vertex.z);
    gl_Position = ubo.wvp * vec4(pos, 1.0);

    texCoord = vec2(vertex.u, vertex.v);
}
