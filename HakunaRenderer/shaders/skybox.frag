#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive :enable
layout(location = 0) in vec3 fragWorldPos;


layout(binding = 1) uniform UniformBufferObject1 {
    vec3 direct;
	vec3 color;
} ubo_direc_light;
layout(binding = 2) uniform UniformBufferObject2 { vec3 cam_world_pos;} ubo_params;
layout(binding = 3) uniform samplerCube skyboxTextureSampler;

layout(location = 0) out vec4 outColor;


#define PI 3.1415926535897932384626433832795


#include "common.inc"

void main() 
{
	vec3 skyDir = normalize(fragWorldPos - ubo_params.cam_world_pos);
	vec3 skyCol = texture(skyboxTextureSampler,skyDir * vec3(1,-1,1)).rgb;
	skyCol = ACESToneMapping(skyCol, 3.0);
	skyCol = pow(skyCol, vec3(1.0f / 2.2f));

	outColor = vec4(skyCol,1);	
}


