#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(early_fragment_tests) in;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) flat in int fragDiffuseTextureIndex;
layout(location = 2) flat in int fragNormalTextureIndex;
layout(location = 3) flat in int fragMetalicRoughness;
layout(location = 4) in vec3 fragNormal;
layout(location = 5) in vec3 fragTangent;
layout(location = 6) in vec3 fragBiTangent;

layout(constant_id = 0) const int MAX_TEXTURE_COUNT = 64;

layout(location = 0) out vec4 gAlbedo;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gPbr;

layout(set = 0, binding = 3) uniform sampler2D textures[];

vec3 calcNormal(vec3 normal, vec2 texCoord, mat3 TBN)
{
    vec3 newNormal = normal;
    if(fragNormalTextureIndex>0)
    {
        newNormal = texture(textures[fragNormalTextureIndex], texCoord).rgb;
        newNormal = normalize(newNormal * 2.0 - 1.0);
        newNormal = normalize(TBN * newNormal);
    }

    return newNormal;
}

void main() {
    gAlbedo = texture(textures[fragDiffuseTextureIndex],fragTexCoord);

    const float alphaThreshold = 0.95f;

    //if (gAlbedo.a < alphaThreshold) {
    //    discard;
    //}

    mat3 TBN = mat3(fragTangent, fragBiTangent, fragNormal);
    vec3 newNormal = calcNormal(fragNormal, fragTexCoord, TBN);

    gNormal = vec4(0.5 * newNormal + 0.5, 1.0);

    float ao = 0.5f;
    float roughnessFactor = 0.0f;
    float metallicFactor = 0.0f;

    if (fragMetalicRoughness > 0) {  // Check if valid texture index
        vec4 metRoughValue = texture(textures[fragMetalicRoughness], fragTexCoord);
        roughnessFactor = metRoughValue.g;  // Roughness in G channel (glTF standard)
        metallicFactor = metRoughValue.b;   // Metallic in B channel (glTF standard)
    }

    
    gPbr = vec4(ao, roughnessFactor, metallicFactor, 1.0f);
}