#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragWorldPos;


layout(binding = 1) uniform UniformBufferObject1 {
    vec3 direct;
	vec3 color;
} ubo_direc_light;
layout(binding = 2) uniform UniformBufferObject2 { vec3 cam_world_pos;} ubo_params;
layout(binding = 3) uniform sampler2D skyboxTextureSampler;

layout(location = 0) out vec4 outColor;


#define PI 3.1415926535897932384626433832795

void main() 
{
	vec3 skyDir = normalize(fragWorldPos - ubo_params.cam_world_pos);
	float theta = acos(skyDir.y);
	float phi = atan(skyDir.z/skyDir.x);
	float u1 = (phi * 0.5 + PI * 0.25) / PI;
	float u2 = (phi * 0.5 + PI * 0.25) / PI + 0.5;
	float u = mix(u1,u2,step(skyDir.x,0));
	float v = theta / PI;
	vec3 skyCol = texture(skyboxTextureSampler, vec2(u,v)).rgb;
    outColor = vec4(skyCol,1);
}


