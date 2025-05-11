// gbuffer.vert
#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

struct Vertex {
    vec3 pos;
    vec3 color;    // Change from 'normal' to 'color'
    vec2 texCoord;
};

layout(std430, binding = 1) readonly buffer VertexBuffer {
    Vertex vertices[];
} vertexBuffer;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) flat out int fragTextureIndex;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragWorldPos;

layout(push_constant) uniform PushConstantData {
    mat4 model;
    int textureIndex;
} push;

void main() {
    Vertex vertex = vertexBuffer.vertices[gl_VertexIndex];
    
    vec4 worldPos = push.model * vec4(vertex.pos, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;
    fragTexCoord = vertex.texCoord;
    fragTextureIndex = push.textureIndex;
    fragWorldPos = worldPos.xyz;
    
    fragNormal = vec3(0.0, 0.0, 1.0);
}