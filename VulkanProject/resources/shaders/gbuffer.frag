#version 450
#extension GL_EXT_nonuniform_qualifier : require
layout(early_fragment_tests) in;

layout(constant_id = 0) const int MAX_TEXTURE_COUNT = 64;

// inputs-------------------------------------------------------------------
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) flat in int fragDiffuseTextureIndex;
layout(location = 2) flat in int fragNormalTextureIndex;
layout(location = 3) flat in int fragMetalicRoughness;
layout(location = 4) in vec3 fragNormal;
layout(location = 5) in vec3 fragTangent;
layout(location = 6) in vec3 fragBiTangent;
//--------------------------------------------------------------------------

// GBuffer outputs----------------------------------------------------------
layout(location = 0) out vec4 gAlbedo;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gPbr;
//--------------------------------------------------------------------------

// all textures-------------------------------------------------------------
layout(set = 1, binding = 0) uniform sampler2D textures[];
//--------------------------------------------------------------------------

// Functions----------------------------------------------------------------
vec3 calcNormal(vec3 normal, vec2 texCoord, mat3 TBN)
{
    vec3 newNormal = normal;
    if(true)
    {
        newNormal = texture(textures[fragNormalTextureIndex], texCoord).rgb;
        newNormal = normalize(newNormal * 2.0 - 1.0);
        newNormal = normalize(TBN * newNormal);
    }
    return newNormal;
}

vec4 packNormalHighPrecision(vec3 normal) {
    normal = normalize(normal);
    
    vec2 normalXY = normal.xy * 0.5 + 0.5;
    
    uvec2 scaled = uvec2(normalXY * 65535.0);
    
    scaled.x = (scaled.x & 0xFFFEu) | (normal.z < 0.0 ? 1u : 0u);

    vec4 packed;
    packed.r = float(scaled.x & 0xFFu) / 255.0;
    packed.g = float((scaled.x >> 8) & 0xFFu) / 255.0;
    packed.b = float(scaled.y & 0xFFu) / 255.0;
    packed.a = float((scaled.y >> 8) & 0xFFu) / 255.0;
    
    return packed;
}
//--------------------------------------------------------------------------

void main() {
    vec4 albedo = texture(textures[fragDiffuseTextureIndex], fragTexCoord);
    const float alphaThreshold = 1.f;
    if (albedo.a < alphaThreshold) {
        discard;
    }
    
    gAlbedo = albedo;
    
    mat3 TBN = mat3(fragTangent, fragBiTangent, fragNormal);
    vec3 newNormal = calcNormal(fragNormal, fragTexCoord, TBN);
    gNormal = packNormalHighPrecision(newNormal);
    
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