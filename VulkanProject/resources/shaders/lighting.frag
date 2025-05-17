#version 450

const float PI = 3.14159265359;

struct Light {
    vec4 position;
    vec4 color;
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
} ubo;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;

	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / max(denom, 0.0001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

vec3 calculateLight(Light light, vec3 lightDirection, vec3 position, vec3 normal, vec3 albedo,
                    float metallic, float roughness, float attenuation)
{
    vec3 result;

    vec3 N = normalize(normal);
    vec3 L = normalize(lightDirection);
    vec3 V = normalize(-position);
    vec3 H = normalize(L+V);


    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    vec3 radiance = light.color.rgb * attenuation;

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 Ks = F;
    vec3 Kd = vec3(1.0) - Ks;
    Kd *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular = numerator / max(denominator, 0.001);

    float NdotL = max(dot(N, L), 0.0);
    Lo += (Kd * albedo / PI + specular) * radiance * NdotL;

    return Lo;
}

vec3 calculateDirectionalLight(Light light, vec3 position, vec3 normal, vec3 color,
float metallic, float roughness) {
    float attenuation = 1.0;
    return calculateLight(light, light.position.xyz, position, normal, color, metallic, roughness,
    attenuation);
}

vec3 calculatePointLight(Light light, vec3 position, vec3 normal, vec3 color,
float metallic, float roughness) {
    vec3 lightDirection = light.position.xyz - position;
    float dist = length(lightDirection);
    float attenuation = 1.0 / (dist * dist);
    return calculateLight(light, lightDirection, position, normal, color, metallic, roughness,
    attenuation);
}

vec3 getCameraPositionFromViewMatrix(mat4 viewMatrix) {
    mat4 invView = inverse(viewMatrix);
    return invView[3].xyz;
}

vec3 GetWorldPositionFromDepth(float depth, vec2 texCoord) {
    // Convert to NDC
    vec4 clipSpace;
    clipSpace.xy = texCoord * 2.0 - 1.0;  // Convert UV to NDC
    clipSpace.z = depth * 2.0 - 1.0;      // Convert depth to NDC
    clipSpace.w = 1.0;
    
    // Calculate inverse matrices
    mat4 invProj = inverse(ubo.proj);
    mat4 invView = inverse(ubo.view);
    
    // Convert to view space
    vec4 viewSpace = invProj * clipSpace;
    viewSpace /= viewSpace.w;
    
    // Convert to world space
    vec4 worldSpace = invView * viewSpace;
    
    return worldSpace.xyz;
}


void main(){
   vec3 albedo = texture(gAlbedo, fragTexCoord).rgb;
   vec3 normal = texture(gNormal, fragTexCoord).rgb;
   vec3 pbr = texture(gPbr, fragTexCoord).rgb;
   float ao = pbr.r;
   float roughness = max(pbr.g, 0.05);
   float metallic = pbr.b;

   float depth = texture(depthSampler, fragTexCoord).r;
   vec3 worldPos = GetWorldPositionFromDepth(depth, fragTexCoord);

   if (depth >= 0.9999) {
       outColor = vec4(0.0, 0.0, 0.0, 1.0);
       return;
   }

   vec3 lightColor = vec3(0.0);
   vec3 ambientColor = vec3(0.5);
   
   vec3 ambientLightColor = vec3(0.2f, 0.2f, 0.2f);

   Light light;
   light.position = vec4(0.f, 1.f, 0.0f, 1.0);  // Direction is negative of light direction
   light.color = vec4(1.0, 0.0, 0.0, 1.0);

   if(light.position.w == 0.0) { // Directional light)
       lightColor += calculateDirectionalLight(light, worldPos, normal, albedo, metallic, roughness);
   }
   else { // Point light
       lightColor += calculatePointLight(light, worldPos, normal, albedo, metallic, roughness);
   }
    vec3 ambient = ambientLightColor * albedo * ao;
    
    outColor = vec4(ambient + lightColor, 1.0);
}