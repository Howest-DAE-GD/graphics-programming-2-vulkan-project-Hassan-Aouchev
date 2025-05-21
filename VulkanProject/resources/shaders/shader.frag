#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in int fragTextureIndex;

layout(constant_id = 0) const int MAX_TEXTURE_COUNT = 64;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 3) uniform sampler2D textures[];

void main() {
    outColor = texture(textures[fragTextureIndex],fragTexCoord);
}