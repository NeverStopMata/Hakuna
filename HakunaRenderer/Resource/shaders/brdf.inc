// Normal Distribution function --------------------------------------
#ifndef BRDF_HEAD
#define BRDF_HEAD
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
#endif