#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 direct;
	vec3 color;
    vec3 cam_world_pos;
	float max_reflection_lod;
	float game_time;
} ubo_global_params;

// layout(push_constant) uniform PushConsts {
// 	mat4 model;
// 	mat4 model_for_normal;
// } pushConsts;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBiTangent;
layout(location = 5) in vec2 inTexCoord;

layout(location = 0) out vec3 fragWorldPos;

void main() {
    vec4 worldPos = vec4(inPosition, 1.0);
    gl_Position   = ubo_global_params.proj * ubo_global_params.view * worldPos;
	fragWorldPos  = worldPos.xyz;
}