// gbuffer.vert
#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

struct Vertex {
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
    vec3 tangent;
    vec3 biTangent;
};

layout(std430, binding = 1) readonly buffer VertexBuffer {
    Vertex vertices[];
} vertexBuffer;


struct Material {
    int baseColorTextureIndex;
    int normalTextureIndex;
    int metallicRoughnessTextureIndex;
    int useTextureFlags;
};

layout(std430, binding = 2) readonly buffer MaterialBuffer {
    Material materials[];
}materialBuffer;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out int fragDiffuseTextureIndex;
layout(location = 2) out int fragNormalTextureIndex;
layout(location = 3) out int fragMetalicRoughness;
layout(location = 4) out vec3 fragNormal;
layout(location = 5) out vec3 fragTangent;
layout(location = 6) out vec3 fragBiTangent;

layout(push_constant) uniform PushConstantData {
    mat4 model;
    int meshIndex;
} push;

void main() {
    Vertex vertex = vertexBuffer.vertices[gl_VertexIndex];
    Material material = materialBuffer.materials[push.meshIndex];
    
    fragTexCoord = vertex.texCoord;
    
    fragDiffuseTextureIndex = material.baseColorTextureIndex;
    fragNormalTextureIndex = material.normalTextureIndex;
    fragMetalicRoughness = material.metallicRoughnessTextureIndex;
    
    mat4 modelViewMatrix = ubo.view * push.model;

    fragTangent = normalize(modelViewMatrix * vec4(vertex.tangent,0.0)).xyz;
    fragBiTangent = normalize(modelViewMatrix * vec4(vertex.biTangent,0.0)).xyz;
    fragNormal = normalize(modelViewMatrix * vec4(vertex.normal,0.0)).xyz;
    
    gl_Position = ubo.proj * modelViewMatrix * vec4(vertex.pos,1.f);
}