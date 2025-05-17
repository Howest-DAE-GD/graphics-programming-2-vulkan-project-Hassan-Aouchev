#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

struct Vertex {
    vec3 pos;
    vec3 color;
    vec2 texCoord;
    vec3 tangent;
    vec3 bitTangent;
};

struct Material {
    int baseColorTextureIndex;
    int normalTextureIndex;
    int metallicRoughnessTextureIndex;
    int useTextureFlags;
};

layout(std430, binding = 1) readonly buffer VertexBuffer {
    Vertex vertices[];
} vertexBuffer;

layout(std430, binding = 2) readonly buffer MaterialBuffer {
    Material materials[];
}materialBuffer;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) flat out int fragTextureIndex;

layout(push_constant) uniform PushConstantData {
    mat4 model;
    int meshIndex;
} push;

void main() {
    Vertex vertex = vertexBuffer.vertices[gl_VertexIndex];
    Material material = materialBuffer.materials[push.meshIndex];
    
    gl_Position = ubo.proj * ubo.view * push.model * vec4(vertex.pos, 1.0);
    fragColor = vertex.color;
    fragTexCoord = vertex.texCoord;
    fragTextureIndex = material.normalTextureIndex;
}