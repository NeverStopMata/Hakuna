#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive :enable
layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec3 fragBiTangent;

layout(binding = 1) uniform UniformBufferObject1 {
    vec3 direct;
	vec3 color;
} ubo_direc_light;
layout(binding = 2) uniform UniformBufferObject2 { vec3 cam_world_pos;} ubo_params;
layout(binding = 3) uniform sampler2D baseColorSampler;
layout(binding = 4) uniform sampler2D normalSampler;
layout(binding = 5) uniform sampler2D metallicSampler;
layout(binding = 6) uniform sampler2D roughnessSampler;
layout(binding = 7) uniform sampler2D occlusionSampler;
layout(binding = 8) uniform samplerCube envSpecularSampler;
layout(binding = 9) uniform samplerCube envDiffuseSampler;
layout(binding = 10) uniform sampler2D envBRDFLutSampler;


layout(location = 0) out vec4 outColor;



#include "common.inc"
#include "brdf.inc"


vec3 prefilteredReflection(vec3 R, float roughness)
{
	const float MAX_REFLECTION_LOD = 9.0; // todo: param/const
	float lod = roughness * (MAX_REFLECTION_LOD-1);
	// float lodf = floor(lod);
	// float lodc = ceil(lod);
	// vec3 a = textureLod(prefilteredMap, R, lodf).rgb;
	// vec3 b = textureLod(prefilteredMap, R, lodc).rgb;
	//return mix(a, b, lod - lodf);
	return textureLod(envSpecularSampler, R * vec3(1,-1,1), lod).rgb;
}


vec3 GetEnvLighting(vec3 N, vec3 V, vec3 R,vec3 F0, float AO,float metallic, float roughness, vec3 albedo)
{
	vec3 irradiance = texture(envDiffuseSampler, N).rgb;
	vec3 diffuse = irradiance * albedo;	
	vec3 F = F_SchlickR(max(dot(N, V), 0.0), F0, roughness);
	vec3 reflection = prefilteredReflection(R, roughness).rgb;
	vec2 brdf = texture(envBRDFLutSampler, vec2(max(dot(N, V), 0.0), roughness)).rg;
	vec3 specular = reflection * (F * brdf.x + brdf.y);
	vec3 kD = 1.0 - F;
	kD *= 1.0 - metallic;	  
	vec3 ambient = (kD * diffuse + specular) * AO;//matatodo : use disney diffuse term instead.
	return ambient;
}
vec3 GetNormal()
{
	mat3 TBN = mat3(fragTangent, fragBiTangent, fragNormal);
	vec3 tangentNormal = texture(normalSampler, fragTexCoord).xyz * 2 - 1;
    return normalize(TBN * tangentNormal);
}

vec3 GetDirectLightContribution(vec3 L, vec3 V, vec3 N, vec3 F0, float metallic, float roughness, vec3 albedo)
{
	// Precalculate vectors and dot products	
	vec3 H = normalize (V + L);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotVH = dot(V, H);
	// Light color fixed
	vec3 lightColor = vec3(1.0);

	vec3 color = vec3(0.0);

	if (dotNL > 0.0) {
		// D = Normal distribution (Distribution of the microfacets)
		float D = D_GGX(dotNH, roughness); 
		// G = Geometric shadowing term (Microfacets shadowing)
		float V = Vis_SmithJoint(dotNL, dotNV, roughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		vec3 F = F_Schlick(dotVH, F0);		
		vec3 spec = D * F * V ;// (4.0 * dotNL * dotNV + 0.001)		
		vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);			
		color += (kD * albedo / PI + spec) * dotNL;
	}
	return color;
}

void main() 
{
    vec3 N = GetNormal();
	vec3 V = normalize(ubo_params.cam_world_pos - fragWorldPos);
	vec3 R = reflect(-V, N); 
	vec3 L = normalize(ubo_direc_light.direct);
	vec3 albedo     = pow(texture(baseColorSampler, fragTexCoord).rgb, vec3(2.2));
	float metallic  = texture(metallicSampler, fragTexCoord).r;
	float roughness = texture(roughnessSampler,fragTexCoord).r;
	float occlusion = texture(occlusionSampler,fragTexCoord).r;
	
	vec3 F0 = mix(vec3(0.04), albedo, metallic);

	vec3 color = GetDirectLightContribution(L, V, N, F0, metallic, roughness, albedo);
	color += GetEnvLighting(N, V, R,F0, occlusion,metallic, roughness, albedo);
	// Tone mapping
	//color = Uncharted2Tonemap(color * uboParams.exposure);
	//color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));	
	// Gamma correction
	color = pow(color, vec3(1.0f / 2.2f));
    outColor = vec4(color,1);//texture(baseColorSampler, fragTexCoord);
}