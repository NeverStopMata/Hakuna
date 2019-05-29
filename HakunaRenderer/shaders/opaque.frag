#version 450
#extension GL_ARB_separate_shader_objects : enable

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
layout(binding = 8) uniform sampler2D irradianceMapSampler;

layout(location = 0) out vec4 outColor;


#define PI     3.1415926535897932384626433832795
#define INV_PI 0.31830988618

// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness)
{
	float alpha = max(roughness * roughness, 0.002);
	float alpha2 = alpha * alpha;
	float denom = dotNH * (dotNH * alpha2 - dotNH) + 1.0;
	return (alpha2)/(PI * denom*denom); 
}

// Geometric Shadowing function --------------------------------------
float Vis_SmithJoint(float dotNL, float dotNV, float roughness )
{

    // Approximation of the above formulation (simplify the sqrt, not mathematically correct but close enough)
    float a = max(roughness * roughness, 0.002);
    float lambdaV = dotNL * (dotNV * (1 - a) + a);
    float lambdaL = dotNV * (dotNL * (1 - a) + a);
    return 0.5f / (lambdaV + lambdaL + 1e-5f);
}

// Fresnel function ----------------------------------------------------
vec3 F_Schlick(float dotVH, vec3 F0)
{
	float Fc = pow( 1 - dotVH, 5.0 );		
	
	return F0 * (1 - Fc) + clamp(50.0 * F0.g, 0.0, 1.0) * Fc;
}
vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 GetEnvDiffuse(vec3 N, vec3 F0, float metallic, vec3 albedo)
{
	float theta = acos(N.y);
	float phi = atan(N.z / N.x);
	float u1 = (phi * 0.5 + PI * 0.25) / PI;
	float u2 = (phi * 0.5 + PI * 0.25) / PI + 0.5;
	float u = mix(u1,u2,step(N.x,0));
	float v = theta / PI;
	vec3 irradiance = pow(texture(irradianceMapSampler,vec2(u,v)).rgb,vec3(2.2));
	vec3 kD = (vec3(1) - F0) * (1 - metallic);
	return albedo * irradiance * kD * INV_PI;
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
	color += GetEnvDiffuse(N, F0, metallic, albedo) * occlusion;
	// Tone mapping
	//color = Uncharted2Tonemap(color * uboParams.exposure);
	//color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));	
	// Gamma correction
	color = pow(color, vec3(1.0f / 2.2f));
    outColor = vec4(color,1);//texture(baseColorSampler, fragTexCoord);
}