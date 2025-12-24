#version 460

struct VertexData {
    float x;
    float y;
    float z;

    float u;
    float v;
};

layout(binding = 0) readonly buffer Vertices{ VertexData vertices[]; }  in_vertices;

void main() {
    VertexData vertex = in_vertices.vertices[gl_VertexIndex];
    vec3 pos = vec3(vertex.x, vertex.y, vertex.z);
    gl_Position = vec4(pos, 1.0);
}
