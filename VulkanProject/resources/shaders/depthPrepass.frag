#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) flat in int fragAlphaTextureIndex;
layout(location = 2) flat in float fragAlphaCutoff;
layout(location = 3) flat in int fragHasAlpha;

// Alpha textures (Set 1, Binding 0)
layout(set = 1, binding = 0) uniform sampler2D alphaTextures[];

void main() {
    if (fragHasAlpha > 0) {
        float alpha = texture(alphaTextures[fragAlphaTextureIndex], fragTexCoord).a;
        if (alpha < fragAlphaCutoff) {
            discard;
        }
    }
}