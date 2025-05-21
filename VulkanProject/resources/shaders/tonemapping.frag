#version 450

layout(binding = 0) uniform sampler2D hdrSampler;

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 fragTexCoord;

void main(){
   outColor = texture(hdrSampler,fragTexCoord);
}