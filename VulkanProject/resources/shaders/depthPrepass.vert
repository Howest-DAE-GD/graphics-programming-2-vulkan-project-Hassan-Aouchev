#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    ivec2 resolution;
    vec3 cameraPosition;
} ubo;

// Vertex structure definition
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
    int hasAlphaMask;
    int alphaTextureIndex;
    float alphaCutoff;
};

// Set 0, Binding 1: Vertex buffer (universal)
layout(set = 0, binding = 1, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
} vertexBuffer;

// Set 0, Binding 2: Material buffer (universal)
layout(set = 0, binding = 2, std430) readonly buffer MaterialBuffer {
    Material materials[];
} materialBuffer;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) flat out int fragAlphaTextureIndex;
layout(location = 2) flat out float fragAlphaCutoff;
layout(location = 3) flat out int fragHasAlpha;

// Push constants
layout(push_constant) uniform PushConstantData {
    mat4 model;
    int meshIndex;
} push;

void main() {
    Vertex vertex = vertexBuffer.vertices[gl_VertexIndex];
    Material material = materialBuffer.materials[push.meshIndex];
    
    fragTexCoord = vertex.texCoord;
    
    fragHasAlpha = material.hasAlphaMask;
    
    if (material.hasAlphaMask > 0) {
        fragAlphaTextureIndex = material.alphaTextureIndex;
        
        fragAlphaCutoff = material.alphaCutoff;
    } else {
        fragAlphaTextureIndex = 0;
        fragAlphaCutoff = 0.5;
    }
    
    gl_Position = ubo.proj * ubo.view * push.model * vec4(vertex.pos, 1.0);
}