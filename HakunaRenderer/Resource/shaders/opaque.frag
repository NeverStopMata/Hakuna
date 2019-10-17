#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive :enable
layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec3 fragBiTangent;
layout(location = 5) in vec3 fragWorldViewDirection;

layout(binding = 1) uniform UniformBufferObject1 {
    vec3 direct;
	vec3 color;
} ubo_direc_light;
layout(binding = 2) uniform UniformBufferObject2 { 
	vec3 cam_world_pos;
	float max_reflection_lod;
} ubo_params;
layout(binding = 3) uniform sampler2D baseColorSampler;
layout(binding = 4) uniform sampler2D normalSampler;
layout(binding = 5) uniform sampler2D metallicSampler;
layout(binding = 6) uniform sampler2D roughnessSampler;
layout(binding = 7) uniform sampler2D occlusionSampler;
layout(binding = 8) uniform samplerCube envDiffuseSampler;
layout(binding = 9) uniform samplerCube envSpecularSampler;
layout(binding = 10) uniform sampler2D envBRDFLutSampler;


layout(location = 0) out vec4 outColor;



#include "common.inc"
#include "brdf.inc"


vec3 prefilteredReflection(vec3 N, vec3 R, float roughness)
{
	float lod = roughness * (ubo_params.max_reflection_lod-1);
	float scale = pow(2,lod);
	float NoR = dot(N,R);
	vec3 dRdx;
	vec3 dRdy;
	if(NoR < 0.5)
	{
		vec3 biTangent = normalize(N / NoR - R);
		vec3 tangent = cross(biTangent,R);
		dRdx = tangent * scale * 0.0045 * (0.9*NoR + 0.1);
		dRdy = biTangent * scale * 0.0045 * (-3*NoR + 4);
		return textureGrad(envSpecularSampler, R * vec3(1,-1,1), dRdx  * vec3(1,-1,1) ,dRdy  * vec3(1,-1,1)).rgb;
	}
	else
	{
		return textureLod(envSpecularSampler, R * vec3(1,-1,1), lod).rgb;
	}
}


vec3 GetEnvLighting(vec3 N, vec3 V, vec3 R,vec3 F0, float AO,float metallic, float roughness, vec3 albedo)
{
	
	vec3 F = F_SchlickR(max(dot(N, V), 0.0), F0, roughness);
	vec3 reflection = prefilteredReflection(N, R, roughness).rgb;
	vec2 brdf = texture(envBRDFLutSampler, vec2(max(dot(N, V), 0.0), 1-roughness)).rg;
	vec3 specular = reflection * (F * brdf.x + brdf.y);
	vec3 kD = 1.0 - F;
	kD *= 1.0 - metallic;	  

	vec3 irradiance = texture(envDiffuseSampler, N * vec3(1,-1,1)).rgb;
	vec3 diffuse = irradiance * albedo;	

	vec3 ambient = (kD * diffuse + specular) * AO;//matatodo : use disney diffuse term instead.
	return ambient;
}
vec3 GetNormal()
{
	//return normalize(fragNormal);
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

// From http://filmicgames.com/archives/75
vec3 Uncharted2Tonemap(vec3 x)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main() 
{
    vec3 N = GetNormal();
	vec3 V = normalize(fragWorldViewDirection);
	vec3 R = reflect(-V, N); 
	vec3 L = normalize(ubo_direc_light.direct);
	vec3 albedo     = pow(texture(baseColorSampler, fragTexCoord).rgb, vec3(2.2));
	float metallic  = texture(metallicSampler, fragTexCoord).r;
	float roughness = texture(roughnessSampler,fragTexCoord).r;
	//use to debug
	// float metallicFlag = step(0.5,metallic);
	// roughness = mix(roughness,0.0,metallicFlag);
	// albedo =  mix(albedo,vec3(0.7,0.7,0.7),metallicFlag);

	// roughness = 0.1;
	// metallic = 1;
	// albedo = vec3(0.99,0.99,0.99);
	//end:use to debug

	float occlusion = texture(occlusionSampler,fragTexCoord).r;
	
	vec3 F0 = mix(vec3(0.04), albedo, metallic);
	vec3 color = GetDirectLightContribution(L, V, N, F0, metallic, roughness, albedo);
	color += GetEnvLighting(N, V, R,F0, occlusion,metallic, roughness, albedo);
	// Tone mapping

	// color = Uncharted2Tonemap(color * 4.5);
	// color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));	

	color = ACESToneMapping(color, 3.0);

	//Gamma correction
	color = pow(color, vec3(1.0f / 2.2f));
    outColor = vec4(color,1);//texture(baseColorSampler, fragTexCoord);
}