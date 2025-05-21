#version 450

const float PI = 3.14159265359;

struct Light {
    vec4 position;
    vec4 color;
    float lumen;
    float lux;
};

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D gAlbedo;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gPbr;
layout(binding = 3) uniform sampler2D depthSampler;

layout(binding = 4) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    ivec2 resolution;
    vec3 cameraPosition;
} ubo;

layout(set = 0, binding = 5, std430) readonly buffer LightingBuffer {
    Light light[];
} lightBuffer;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness;
    const bool squareRoughness = false;
    if(squareRoughness)
    a = roughness * roughness;
	float a2 = a * a;

	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

vec3 GetWorldPositionFromDepth(float depth, ivec2 texCoord) {

    vec2 ndc = vec2(
         (2.0 * (texCoord.x + 0.5)) / ubo.resolution.x - 1.0,
         (2.0 * (texCoord.y + 0.5)) / ubo.resolution.y - 1.0);

    const vec4 clipPos = vec4(ndc,depth,1.0f);
    
    // Calculate inverse matrices
    mat4 invProj = inverse(ubo.proj);
    mat4 invView = inverse(ubo.view);
    
    // Convert to view space
    vec4 viewSpace = invProj * clipPos;
    viewSpace /= viewSpace.w;
    
    // Convert to world space
    vec4 worldSpace = invView * viewSpace;
    
    return worldSpace.xyz;
}

vec3 unpackNormalHighPrecision(vec4 packed) {

    uint xLow = uint(packed.r * 255.0);
    uint xHigh = uint(packed.g * 255.0);
    uint yLow = uint(packed.b * 255.0);
    uint yHigh = uint(packed.a * 255.0);
    
    float signZ = (xLow & 1u) != 0u ? -1.0 : 1.0; // i stored the sign of z in the xLow byte cause it gets lost when packing only x and y

    xLow &= 0xFEu;
    
    uvec2 scaled = uvec2(
        xLow | (xHigh << 8),
        yLow | (yHigh << 8)
    );
    
    vec2 normalXY = vec2(scaled) / 65535.0;
    
    normalXY = normalXY * 2.0 - 1.0;
    
    float normalZ = sqrt(max(0.0, 1.0 - dot(normalXY, normalXY)));// only positive z here
    normalZ *= signZ;// now its negative depending on the xLow
    
    return normalize(vec3(normalXY, normalZ));
}

void main(){
   vec3 albedo = texture(gAlbedo, fragTexCoord).rgb;
   vec4 packedNormal = texture(gNormal, fragTexCoord);
   
   vec3 normal = unpackNormalHighPrecision(packedNormal);

   vec3 pbr = texture(gPbr, fragTexCoord).rgb;
   float ao = pbr.r;
   float roughness = max(pbr.g, 0.05);
   float metallic = pbr.b;

   float depth = texelFetch(depthSampler, ivec2(gl_FragCoord.xy),0).r;
   vec3 worldPos = GetWorldPositionFromDepth(depth, ivec2(gl_FragCoord.xy));

   vec3 ambientColor = vec3(0.0);
   
   vec3 ambientLightColor = vec3(0.2f, 0.2f, 0.2f);

   vec3 V = normalize(ubo.cameraPosition - worldPos);


   vec3 Lo = vec3(0.0);
   for (int i = 0; i < 3 ;i++){
        vec3 L;
        float attenuation;
        if(lightBuffer.light[i].position.w == 1.f)
        {
            L = normalize(lightBuffer.light[i].position.xyz - worldPos);
            float distance = length(lightBuffer.light[i].position.xyz - worldPos);
            attenuation = 1.0 / (distance * distance);
        }
        else
        {
            L = normalize(lightBuffer.light[i].position.xyz);
            attenuation = 1.0;
        }
        vec3 H = normalize(V+L);
        vec3 radiance = lightBuffer.light[i].position.w == 0.f ? lightBuffer.light[i].color.rgb * lightBuffer.light[i].lux :lightBuffer.light[i].color.rgb * attenuation*(lightBuffer.light[i].lumen/(4.0*PI));

        float NDF = DistributionGGX(normal, H, roughness);
        float G = GeometrySmith(normal, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), vec3(0.04));

        vec3 Ks = F;
        vec3 Kd = vec3(1.0) - Ks;
        Kd *= 1.0 - metallic;

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        float NdotL = max(dot(normal, L), 0.0);
        Lo += (Kd * albedo / PI + specular) * radiance * NdotL;
   }

    vec3 ambient = ambientColor * albedo * ao;
    vec3 color = ambient + Lo;
    
    outColor = vec4(color, 1.0);

}