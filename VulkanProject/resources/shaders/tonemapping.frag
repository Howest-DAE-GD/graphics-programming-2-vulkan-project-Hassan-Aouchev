#version 450

#define SUNNY_16

layout(binding = 0) uniform sampler2D hdrSampler;

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 fragTexCoord;

float CalculateEV100FromPhysicalCamera(float aperture, float shutterSpeed, float iso)
{
    return log2(pow(aperture,2) / shutterSpeed * 100 / iso);
}

float CalculateEV100ToExposure(float ev100)
{
    const float maxLuminance = 1.2 * pow(2.0,ev100);
    return 1.0 / max(maxLuminance,0.0001);
}

float CalculateEV100FromAverageLuminance(float averageLuminance)
{
    const float K = 12.5;
    return log2((averageLuminance*100.0)/K);
}

vec3 Uncharted2TonemappingCurve(vec3 color)
{
    const float a = 0.15;
	const float b = 0.50;
	const float c = 0.10;
	const float d = 0.20;
	const float e = 0.02;
	const float f = 0.30;

	return ((color * (a*color+c*b)+d*e)/(color*(a*color+b)+d*f))- e/f;
}

vec3 Uncharted2Tonemapping(vec3 color)
{
    const float W = 11.2;
    const vec3 curvedColor = Uncharted2TonemappingCurve(color);
    float whiteScale = 1.0 / Uncharted2TonemappingCurve(vec3(W)).r;
    return clamp(curvedColor * whiteScale,0.0,1.0);
}

void main(){
   vec3 color = texture(hdrSampler,fragTexCoord).rgb;

   #ifdef SUNNY_16
        float aperture = 5.0;
        float iso = 100.0;
        float shutterSpeed = 1.0/200.0;
   #endif

   #ifdef INDOOR
        float aperture = 1.4;
        float iso = 1600.0;
        float shutterSpeed = 1.0/60.0;
   #endif
   const float EV100_HardCoded = -4.f;
   const float EV100_PhysicalCamera = CalculateEV100FromPhysicalCamera(aperture, shutterSpeed, iso);

   float exposure = CalculateEV100ToExposure(EV100_HardCoded);

   color = Uncharted2Tonemapping(color * exposure);

   outColor = vec4(color, 1.0);
}