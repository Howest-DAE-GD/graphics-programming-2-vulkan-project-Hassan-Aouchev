#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) flat in int fragTextureIndex;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPos;

layout(constant_id = 0) const int MAX_TEXTURE_COUNT = 64;

layout(location = 0) out vec4 gAlbedo;
layout(location = 1) out vec4 gNormal;

layout(set = 0, binding = 2) uniform sampler2D textures[];

void main() {
    vec4 texColor = texture(textures[fragTextureIndex], fragTexCoord);
    
    if(texColor.a < 0.95) {
        discard;
    }
    
    gAlbedo = texColor;
    gNormal = vec4(normalize(fragNormal) * 0.5 + 0.5, 1.0); // Pack normals to [0,1]
}