#ifndef COMMON_HEAD
#define COMMON_HEAD

#ifndef PI
#define PI     3.1415926535897932384626433832795
#endif

#ifndef INV_PI
#define INV_PI 0.31830988618
#endif


vec3 AffineTransformation(vec3 originalVec,vec3 axis, float scale)
{
    float projectLength = dot(originalVec,axis);
    float deltaLength = scale * projectLength - projectLength;
    return axis * deltaLength + originalVec;
}

vec3 ACESToneMapping(vec3 color, float adapted_lum)
{
	const float A = 2.51f;
	const float B = 0.03f;
	const float C = 2.43f;
	const float D = 0.59f;
	const float E = 0.14f;

	color *= adapted_lum;
	return (color * (A * color + B)) / (color * (C * color + D) + E);
}

#endif