#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
	mat4 model_for_normal;
    mat4 view;
    mat4 proj;
} ubo_mvp;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBiTangent;
layout(location = 5) in vec2 inTexCoord;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec3 fragBiTangent;

void main() {
    vec4 worldPos = ubo_mvp.model * vec4(inPosition, 1.0);
    gl_Position   = ubo_mvp.proj * ubo_mvp.view * worldPos;
	fragWorldPos  = worldPos.xyz;
    fragTexCoord  = inTexCoord;
	fragNormal    = (ubo_mvp.model_for_normal * vec4(inNormal, 0.0)).xyz;
	fragTangent   = (ubo_mvp.model * vec4(inTangent,   0.0)).xyz;
	fragBiTangent = (ubo_mvp.model * vec4(inBiTangent, 0.0)).xyz;
}