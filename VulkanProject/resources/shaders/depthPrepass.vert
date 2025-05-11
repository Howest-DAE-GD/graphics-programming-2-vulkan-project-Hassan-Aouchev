#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

struct Vertex {
    vec3 pos;
    vec3 color;
    vec2 texCoord;
};

layout(std430, binding = 1) readonly buffer VertexBuffer {
    Vertex vertices[];
} vertexBuffer;

layout(push_constant) uniform PushConstantData {
    mat4 model;
    int textureIndex;
} push;

void main() {
    Vertex vertex = vertexBuffer.vertices[gl_VertexIndex];
    gl_Position = ubo.proj * ubo.view * push.model * vec4(vertex.pos, 1.0);
}