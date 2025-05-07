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

layout(std430, binding = 2) readonly buffer VertexBuffer {
    Vertex vertices[];
} vertexBuffer;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) flat out int fragTextureIndex;

layout(push_constant) uniform PushConstantData {
    mat4 model;
    int textureIndex;
} push;

void main() {
    Vertex vertex = vertexBuffer.vertices[gl_VertexIndex];
    
    gl_Position = ubo.proj * ubo.view * push.model * vec4(vertex.pos, 1.0);
    fragColor = vertex.color;
    fragTexCoord = vertex.texCoord;
    fragTextureIndex = push.textureIndex;
}