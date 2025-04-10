#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in int fragTextureIndex;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D textures[64];


void main() {
    outColor = texture(textures[fragTextureIndex],fragTexCoord);
}