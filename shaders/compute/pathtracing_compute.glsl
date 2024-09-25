#version 430
precision highp float;
layout(local_size_x = 8, local_size_y = 8) in;

layout(rgba32f, binding = 0) uniform image2D img_output;

uniform vec3 camera;
uniform float iTime;
uniform float iFrame;
uniform vec3 sphereX;
uniform float game_window_x;
uniform float game_window_y;
uniform vec2 iMouse;
uniform float angleX;
uniform float angleY;

uniform int test_int;
uniform float u_fov;

uniform bool w_press;
uniform vec3 cameraPos, cameraFwd, cameraUp, cameraRight, cameraMov;
//layout(binding = 6) uniform samplerCube skybox;
layout(binding = 7) uniform sampler2D equirectangularMap;
layout(binding = 5) uniform sampler3D world;


#define SCENE 8

layout(std430, binding = 4) buffer layoutName
{
	float data_SSBO[];
};


// The minimunm distance a ray must travel before we consider an intersection.
// This is to prevent a ray from intersecting a surface it just bounced off of.
const float c_minimumRayHitTime = 0.001;
const float c_superFar = 10000.0;

const float c_pi = 3.14159265359f;
const float c_twopi = 2.0f * c_pi;

// a pixel value multiplier of light before tone mapping and sRGB
const float c_exposure = 1.0f;

// after a hit, it moves the ray this far along the normal away from a surface.
// Helps prevent incorrect intersections when rays bounce off of objects.
const float c_rayPosNormalNudge = 0.001;

// how many renders per frame - to get around the vsync limitation.
const int c_numRendersPerFrame = 2;
const float c_minCameraAngle = 0.01f;
const float c_maxCameraAngle = (c_pi - 0.01f);
vec3 c_cameraAt = camera;
const float c_cameraDistance = 0.0f;
float FOV = u_fov;


const vec3 light = vec3(0.0, 10.0, 20.0);

struct SMaterialInfo
{
	// Note: diffuse chance is 1.0f - (specularChance+refractionChance)
	vec3  albedo;              // the color used for diffuse lighting
	vec3  emissive;            // how much the surface glows
	float specularChance;      // percentage chance of doing a specular reflection
	float specularRoughness;   // how rough the specular reflections are
	vec3  specularColor;       // the color tint of specular reflections
	float IOR;                 // index of refraction. used by fresnel and refraction.
	float refractionChance;    // percent chance of doing a refractive transmission
	float refractionRoughness; // how rough the refractive transmissions are
	vec3  refractionColor;     // absorption for beer's law    
};

SMaterialInfo GetZeroedMaterial()
{
	SMaterialInfo ret;
	ret.albedo = vec3(0.0f, 0.0f, 0.0f);
	ret.emissive = vec3(0.0f, 0.0f, 0.0f);
	ret.specularChance = 0.0f;
	ret.specularRoughness = 0.0f;
	ret.specularColor = vec3(0.0f, 0.0f, 0.0f);
	ret.IOR = 1.0f;
	ret.refractionChance = 0.0f;
	ret.refractionRoughness = 0.0f;
	ret.refractionColor = vec3(0.0f, 0.0f, 0.0f);
	return ret;
}

struct SRayHitInfo
{
	bool fromInside;
	float dist;
	vec3 normal;
	SMaterialInfo material;
};


float ScalarTriple(vec3 u, vec3 v, vec3 w)
{
	return dot(cross(u, v), w);
}


const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(in vec3 v)
{
	vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}

uint wang_hash(inout uint seed)
{
	seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
	seed *= uint(9);
	seed = seed ^ (seed >> 4);
	seed *= uint(0x27d4eb2d);
	seed = seed ^ (seed >> 15);
	return seed;
}

float RandomFloat01(inout uint state)
{
	return float(wang_hash(state)) / 4294967296.0;
}

vec3 RandomUnitVector(inout uint state)
{
	float z = RandomFloat01(state) * 2.0f - 1.0f;
	float a = RandomFloat01(state) * 2 * 3.14159265359f;
	float r = sqrt(1.0f - z * z);
	float x = r * cos(a);
	float y = r * sin(a);
	return vec3(x, y, z);
}

float FresnelReflectAmount(float n1, float n2, vec3 normal, vec3 incident, float f0, float f90)
{
	// Schlick aproximation
	float r0 = (n1 - n2) / (n1 + n2);
	r0 *= r0;
	float cosX = -dot(normal, incident);
	if (n1 > n2)
	{
		float n = n1 / n2;
		float sinT2 = n * n*(1.0 - cosX * cosX);
		// Total internal reflection
		if (sinT2 > 1.0)
			return f90;
		cosX = sqrt(1.0 - sinT2);
	}
	float x = 1.0 - cosX;
	float ret = r0 + (1.0 - r0)*x*x*x*x*x;

	// adjust reflect multiplier for object reflectivity
	return mix(f0, f90, ret);
}

// Triangle intersection. Returns { t, u, v }
vec3 triIntersect(in vec3 ro, in vec3 rd, in vec3 v0, in vec3 v1, in vec3 v2)
{
	vec3 v1v0 = v1 - v0;
	vec3 v2v0 = v2 - v0;
	vec3 rov0 = ro - v0;

	// The four determinants above have lots of terms in common. Knowing the changing
	// the order of the columns/rows doesn't change the volume/determinant, and that
	// the volume is dot(cross(a,b,c)), we can precompute some common terms and reduce
	// it all to:
	vec3  n = cross(v1v0, v2v0);
	vec3  q = cross(rov0, rd);
	float d = 1.0 / dot(rd, n);
	float u = d * dot(-q, v2v0);
	float v = d * dot(q, v1v0);
	float t = d * dot(-n, rov0);

	if (u<0.0 || v<0.0 || (u + v)>1.0) t = -1.0;

	return vec3(t, u, v);
}

bool testTriangleIntersect(in vec3 ro, in vec3 rd, in vec3 v0, in vec3 v1, in vec3 v2, inout SRayHitInfo info) {
	vec3 intersectPoint = triIntersect(ro, rd, v0, v1, v2);

	vec3 v1v0 = v1 - v0;
	vec3 v2v0 = v2 - v0;
	vec3 rov0 = ro - v0;

	if (intersectPoint.x > 0.0 && intersectPoint.x > c_minimumRayHitTime && intersectPoint.x < info.dist) {
		info.dist = intersectPoint.x;
		info.normal = normalize(cross(v1v0, v2v0));
		return true;
	}

	return false;
}

bool TestCubeTrace(in vec3 rayPos, in vec3 rayDir, inout SRayHitInfo info, in vec3 vmin, in vec3 vmax) {
	
	vec3 tmin = (vmin - rayPos) / rayDir;
	vec3 tmax = (vmax - rayPos) / rayDir;

	vec3 t1 = min(tmin, tmax);
	vec3 t2 = max(tmin, tmax);

	float tnear = max(max(t1.x, t1.y), t1.z);
	float tfar = min(min(t2.x, t2.y), t2.z);

	if (tnear > 0.0 && tnear < tfar && tnear < info.dist) {
		info.dist = tnear;
		vec3 newPos = (rayPos + rayDir * info.dist);
		if (newPos.x < vmin.x + 0.0001) info.normal = vec3(-1.0, 0.0, 0.0);
		else if(newPos.x > vmax.x - 0.0001) info.normal = vec3(1.0, 0.0, 0.0);
		else if(newPos.y < vmin.y + 0.0001) info.normal = vec3(0.0, 1.0, 0.0);
		else if(newPos.y > vmax.y - 0.0001) info.normal = vec3(0.0, -1.0, 0.0);
		else if(newPos.z < vmin.z + 0.0001) info.normal = vec3(0.0, 0.0, -1.0);
		else {
			info.normal =  vec3(0.0, 0.0, 1.0);
		}
			
		//rayPos.z += 1.0;
		return true;
	}
	

	return false;
}


bool TestQuadTrace(in vec3 rayPos, in vec3 rayDir, inout SRayHitInfo info, in vec3 a, in vec3 b, in vec3 c, in vec3 d)
{
	// calculate normal and flip vertices order if needed
	vec3 normal = normalize(cross(c - a, c - b));
	if (dot(normal, rayDir) > 0.0)
	{
		normal *= -1.0;

		vec3 temp = d;
		d = a;
		a = temp;

		temp = b;
		b = c;
		c = temp;
	}

	vec3 p = rayPos;
	vec3 q = rayPos + rayDir;
	vec3 pq = q - p;
	vec3 pa = a - p;
	vec3 pb = b - p;
	vec3 pc = c - p;

	// determine which triangle to test against by testing against diagonal first
	vec3 m = cross(pc, pq);
	float v = dot(pa, m);
	vec3 intersectPos;
	if (v >= 0.0)
	{
		// test against triangle a,b,c
		float u = -dot(pb, m);
		if (u < 0.0) return false;
		float w = ScalarTriple(pq, pb, pa);
		if (w < 0.0) return false;
		float denom = 1.0 / (u + v + w);
		u *= denom;
		v *= denom;
		w *= denom;
		intersectPos = u * a + v * b + w * c;
	}
	else
	{
		vec3 pd = d - p;
		float u = dot(pd, m);
		if (u < 0.0) return false;
		float w = ScalarTriple(pq, pa, pd);
		if (w < 0.0) return false;
		v = -v;
		float denom = 1.0 / (u + v + w);
		u *= denom;
		v *= denom;
		w *= denom;
		intersectPos = u * a + v * d + w * c;
	}

	float dist;
	if (abs(rayDir.x) > 0.1)
	{
		dist = (intersectPos.x - rayPos.x) / rayDir.x;
	}
	else if (abs(rayDir.y) > 0.1)
	{
		dist = (intersectPos.y - rayPos.y) / rayDir.y;
	}
	else
	{
		dist = (intersectPos.z - rayPos.z) / rayDir.z;
	}

	if (dist > c_minimumRayHitTime && dist < info.dist)
	{
		info.dist = dist;
		info.normal = normal;
		return true;
	}

	return false;
}

bool TestSphereTrace(in vec3 rayPos, in vec3 rayDir, inout SRayHitInfo info, in vec4 sphere)
{
	//get the vector from the center of this sphere to where the ray begins.
	vec3 m = rayPos - sphere.xyz;

	//get the dot product of the above vector and the ray's vector
	float b = dot(m, rayDir);

	float c = dot(m, m) - sphere.w * sphere.w;

	//exit if r's origin outside s (c > 0) and r pointing away from s (b > 0)
	if (c > 0.0 && b > 0.0)
		return false;

	//calculate discriminant
	float discr = b * b - c;

	//a negative discriminant corresponds to ray missing sphere
	if (discr < 0.0)
		return false;

	//ray now found to intersect sphere, compute smallest t value of intersection
	bool fromInside = false;
	float dist = -b - sqrt(discr);
	if (dist < 0.0)
	{
		fromInside = true;
		dist = -b + sqrt(discr);
	}

	if (dist > c_minimumRayHitTime && dist < info.dist)
	{
		info.dist = dist;
		info.normal = normalize((rayPos + rayDir * dist) - sphere.xyz) * (fromInside ? -1.0 : 1.0);
		return true;
	}

	return false;
}

bool RayGridIntersect(in vec3 rayPos, in vec3 rayDir, inout float t0, inout float t1, in vec3 gridMin, in vec3 gridMax, in SRayHitInfo info) {

	
	vec3 tmin = (gridMin - rayPos) / rayDir;
	vec3 tmax = (gridMax - rayPos) / rayDir;

	vec3 t4 = min(tmin, tmax);
	vec3 t5 = max(tmin, tmax);

	float tnear = max(max(t4.x, t4.y), t4.z);
	float tfar = min(min(t5.x, t5.y), t5.z);

	if (tnear > 0.0 && tnear < tfar && tnear < info.dist) {
		t0 = tnear;
		t1 = tfar;
		return true;
	}
	return false;
	
}

//slab test
bool RayGridIntersect(in vec3 rayPos, in vec3 rayDir, in SRayHitInfo info, in vec3 gridMin, in vec3 gridMax) {
	vec3 tmin = (gridMin - rayPos) / rayDir;
	vec3 tmax = (gridMax - rayPos) / rayDir;

	vec3 t4 = min(tmin, tmax);
	vec3 t5 = max(tmin, tmax);

	float tnear = max(max(t4.x, t4.y), t4.z);
	float tfar = min(min(t5.x, t5.y), t5.z);

	if (tnear > 0.0 && tnear < tfar && tnear < info.dist) {
		return true;
	}
	return false;
}

void TestVoxelTrace(inout vec3 rayPos, in vec3 rayDir, inout SRayHitInfo hitInfo) {

	float t0;
	float t1;

	const bool gridintersect = RayGridIntersect(rayPos, rayDir, t0, t1, vec3(0.0, 0.0, 0.0), vec3(16.0, 16.0, 16.0), hitInfo);
	if (gridintersect) {

		

	}
}


void TestSceneTrace(in vec3 rayPos, in vec3 rayDir, inout SRayHitInfo hitInfo)
{

	// to move the scene around, since we can't move the camera yet
	vec3 sceneTranslation = sphereX;
	vec4 sceneTranslation4 = vec4(sceneTranslation, 0.0f);
	

	{
		vec3 A = vec3(-50.0f, -12.5f, 50.0f);
		vec3 B = vec3(50.0f, -12.5f, 50.0f);
		vec3 C = vec3(50.0f, -12.5f, -50.0f);
		vec3 D = vec3(-50.0f, -12.5f, -50.0f);
		if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
		{
			vec3 hitPos = rayPos + rayDir * hitInfo.dist;

			//checkerboard texture
			float checkerSize = 0.1;
			float floorColor = mod(floor(checkerSize * hitPos.x) + floor(checkerSize * hitPos.z), 2.0);
			float fin = max(sign(floorColor), 0.0);

			hitInfo.material = GetZeroedMaterial();
			hitInfo.material.albedo = vec3(fin, fin, fin);
		}
	}

	// striped background
	{
		vec3 A = vec3(-25.0f, -1.5f, 5.0f) + sphereX;
		vec3 B = vec3(25.0f, -1.5f, 5.0f) + sphereX;
		vec3 C = vec3(25.0f, -10.5f, 5.0f) + sphereX;
		vec3 D = vec3(-25.0f, -10.5f, 5.0f) + sphereX;
		if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
		{
			hitInfo.material = GetZeroedMaterial();

			vec3 hitPos = rayPos + rayDir * hitInfo.dist;

			float shade = floor(mod(hitPos.x, 1.0) * 2.0f);
			hitInfo.material.albedo = vec3(shade, shade, shade);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.specularChance = 0.02f;
			hitInfo.material.specularRoughness = 0.0;
			hitInfo.material.specularColor = vec3(1.0, 1.0, 1.0) * 0.8f;
			hitInfo.material.IOR = 1.1f;
			hitInfo.material.refractionChance = 0.0f;
			hitInfo.material.refractionRoughness = 0.0;
			hitInfo.material.refractionColor = vec3(0.0f, 0.5f, 1.0f);
		}
	}

	// cieling piece above light
	{
		vec3 A = vec3(-7.5f, 12.5f, 5.0f);
		vec3 B = vec3(7.5f, 12.5f, 5.0f);
		vec3 C = vec3(7.5f, 12.5f, -5.0f);
		vec3 D = vec3(-7.5f, 12.5f, -5.0f);
		if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
		{
			hitInfo.material = GetZeroedMaterial();
			hitInfo.material.albedo = vec3(0.7f, 0.7f, 0.7f);
			
		}
	}

	// light
	/*
	{
		vec3 A = vec3(-5.0f, 12.4f, 2.5f);
		vec3 B = vec3(5.0f, 12.4f, 2.5f);
		vec3 C = vec3(5.0f, 12.4f, -2.5f);
		vec3 D = vec3(-5.0f, 12.4f, -2.5f);
		if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
		{
			hitInfo.material = GetZeroedMaterial();
			hitInfo.material.emissive = vec3(1.0f, 0.9f, 0.7f) * 20.0f;
		}
	}
	*/


#if SCENE == 0

	const int c_numSpheres = 7;
	for (int sphereIndex = 0; sphereIndex < c_numSpheres; ++sphereIndex)
	{
		if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(-18.0f + 6.0f * float(sphereIndex), -8.0f, 0.0f, 2.8f)))
		{
			float r = float(sphereIndex) / float(c_numSpheres - 1) * 0.5f;

			hitInfo.material = GetZeroedMaterial();
			hitInfo.material.albedo = vec3(0.9f, 0.25f, 0.25f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.specularChance = 0.02f;
			hitInfo.material.specularRoughness = r;
			hitInfo.material.specularColor = vec3(1.0f, 1.0f, 1.0f) * 0.8f;
			hitInfo.material.IOR = 1.1f;
			hitInfo.material.refractionChance = 1.0f;
			hitInfo.material.refractionRoughness = r;
			hitInfo.material.refractionColor = vec3(0.0f, 0.5f, 1.0f);
		}
	}

#elif SCENE == 1

	const int c_numSpheres = 7;
	for (int sphereIndex = 0; sphereIndex < c_numSpheres; ++sphereIndex)
	{
		if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(-18.0f + 6.0f * float(sphereIndex), -8.0f, 0.0f, 2.8f)))
		{
			float ior = 1.0f + 0.5f * float(sphereIndex) / float(c_numSpheres - 1);

			hitInfo.material = GetZeroedMaterial();
			hitInfo.material.albedo = vec3(0.9f, 0.25f, 0.25f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.specularChance = 0.02f;
			hitInfo.material.specularRoughness = 0.0f;
			hitInfo.material.specularColor = vec3(1.0f, 1.0f, 1.0f) * 0.8f;
			hitInfo.material.IOR = ior;
			hitInfo.material.refractionChance = 1.0f;
			hitInfo.material.refractionRoughness = 0.0f;
		}
	}

#elif SCENE == 2

	const int c_numSpheres = 7;
	for (int sphereIndex = 0; sphereIndex < c_numSpheres; ++sphereIndex)
	{
		if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(-18.0f + 6.0f * float(sphereIndex), -8.0f, 0.0f, 2.8f)))
		{
			float ior = 1.0f + 1.0f * float(sphereIndex) / float(c_numSpheres - 1);

			hitInfo.material = GetZeroedMaterial();
			hitInfo.material.albedo = vec3(0.9f, 0.25f, 0.25f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.specularChance = 0.02f;
			hitInfo.material.specularRoughness = 0.0f;
			hitInfo.material.specularColor = vec3(1.0f, 1.0f, 1.0f) * 0.8f;
			hitInfo.material.IOR = ior;
			hitInfo.material.refractionChance = 0.0f;
		}
	}

#elif SCENE == 3

	const int c_numSpheres = 7;
	for (int sphereIndex = 0; sphereIndex < c_numSpheres; ++sphereIndex)
	{
		if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(-18.0f + 6.0f * float(sphereIndex), -8.0f, 0.0f, 2.8f)))
		{
			float absorb = float(sphereIndex) / float(c_numSpheres - 1);

			hitInfo.material = GetZeroedMaterial();
			hitInfo.material.albedo = vec3(0.9f, 0.25f, 0.25f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.specularChance = 0.02f;
			hitInfo.material.specularRoughness = 0.0f;
			hitInfo.material.specularColor = vec3(1.0f, 1.0f, 1.0f) * 0.8f;
			hitInfo.material.IOR = 1.1f;
			hitInfo.material.refractionChance = 1.0f;
			hitInfo.material.refractionRoughness = 0.0f;
			hitInfo.material.refractionColor = vec3(1.0f, 2.0f, 3.0f) * absorb;
		}
	}

#elif SCENE == 4

	const int c_numSpheres = 7;
	for (int sphereIndex = 0; sphereIndex < c_numSpheres; ++sphereIndex)
	{
		if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(-18.0f + 6.0f * float(sphereIndex), -9.0f + 0.75f * float(sphereIndex), 0.0f, 2.8f)))
		{
			hitInfo.material = GetZeroedMaterial();
			hitInfo.material.albedo = vec3(0.9f, 0.25f, 0.25f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.specularChance = 0.02f;
			hitInfo.material.specularRoughness = 0.0f;
			hitInfo.material.specularColor = vec3(1.0f, 1.0f, 1.0f) * 0.8f;
			hitInfo.material.IOR = 1.5f;
			hitInfo.material.refractionChance = 1.0f;
			hitInfo.material.refractionRoughness = 0.0f;
		}
	}

#elif SCENE == 5

	const int c_numSpheres = 7;
	for (int sphereIndex = 0; sphereIndex < c_numSpheres; ++sphereIndex)
	{
		if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(-18.0f + 6.0f * float(sphereIndex), -9.0f, 0.0f, 2.8f)))
		{
			float transparency = float(sphereIndex) / float(c_numSpheres - 1);

			hitInfo.material = GetZeroedMaterial();
			hitInfo.material.albedo = vec3(0.9f, 0.25f, 0.25f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.specularChance = 0.02f;
			hitInfo.material.specularRoughness = 0.0f;
			hitInfo.material.specularColor = vec3(1.0f, 1.0f, 1.0f) * 0.8f;
			hitInfo.material.IOR = 1.1f;
			hitInfo.material.refractionChance = 1.0f - transparency;
			hitInfo.material.refractionRoughness = 0.0f;
		}
	}

#elif SCENE == 6

	const int c_numSpheres = 7;
	for (int sphereIndex = 0; sphereIndex < c_numSpheres; ++sphereIndex)
	{
		if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(-18.0f + 6.0f * float(sphereIndex), -8.0f, 00.0f, 2.8f)))
		{
			float r = float(sphereIndex) / float(c_numSpheres - 1) * 0.5f;

			hitInfo.material = GetZeroedMaterial();
			hitInfo.material.albedo = vec3(0.9f, 0.25f, 0.25f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.specularChance = 0.02f;
			hitInfo.material.specularRoughness = r;
			hitInfo.material.specularColor = vec3(1.0f, 1.0f, 1.0f) * 0.8f;
			hitInfo.material.IOR = 1.1f;
			hitInfo.material.refractionChance = 1.0f;
			hitInfo.material.refractionRoughness = r;
			hitInfo.material.refractionColor = vec3(0.0f, 0.0f, 0.0f);
		}
	}

#elif SCENE == 7

const int c_numSpheres = 7;
for (int sphereIndex = 0; sphereIndex < c_numSpheres; ++sphereIndex)
{
	if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(-18.0f + 6.0f * float(sphereIndex), -8.0f, 00.0f, 2.8f)))
	{
		float r = float(sphereIndex) / float(c_numSpheres - 1) * 0.5f;

		hitInfo.material = GetZeroedMaterial();

		vec3 hitPos = rayPos + rayDir * hitInfo.dist;

		float shade = floor(mod(hitPos.x, 1.0f) * 2.0f);
		hitInfo.material.albedo = vec3(shade, shade, shade);
		hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
		hitInfo.material.specularChance = r;
		hitInfo.material.specularRoughness = r;
		hitInfo.material.specularColor = vec3(1.0f, 1.0f, 1.0f);
		hitInfo.material.IOR = 1.1f;
		hitInfo.material.refractionChance = 0.0f;
		hitInfo.material.refractionRoughness = r;
		hitInfo.material.refractionColor = vec3(0.0f, 0.0f, 0.0f);
	}
}

#elif SCENE == 8

	if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(-18.0f + 6.0f * float(0), -8.0f, 00.0f, 2.8f)))
	{

		hitInfo.material = GetZeroedMaterial();

		vec3 hitPos = rayPos + rayDir * hitInfo.dist;
		hitInfo.material.albedo = vec3(0.9, 0.9, 0.9);
		hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
		hitInfo.material.specularChance = 0.4;
		hitInfo.material.specularRoughness = 0.0;
		hitInfo.material.specularColor = vec3(1.0f, 1.0f, 1.0f);
		hitInfo.material.IOR = 1.1f;
		hitInfo.material.refractionChance = 0.0f;
		hitInfo.material.refractionRoughness = 0.0;
		hitInfo.material.refractionColor = vec3(0.0f, 0.0f, 0.0f);
	}

	if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(-18.0f + 6.0f * float(1), -8.0f, 00.0f, 2.8f)))
	{

		hitInfo.material = GetZeroedMaterial();

		vec3 hitPos = rayPos + rayDir * hitInfo.dist;
		hitInfo.material.albedo = vec3(0.9, 0.9, 0.9);
		hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
		hitInfo.material.specularChance = 1.0;
		hitInfo.material.specularRoughness = 0.0;
		hitInfo.material.specularColor = vec3(1.0f, 1.0f, 1.0f);
		hitInfo.material.IOR = 1.1f;
		hitInfo.material.refractionChance = 0.0f;
		hitInfo.material.refractionRoughness = 0.0;
		hitInfo.material.refractionColor = vec3(0.0f, 0.0f, 0.0f);
	}

	if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(-18.0f + 6.0f * float(2), -8.0f, 00.0f, 2.8f)))
	{

		hitInfo.material = GetZeroedMaterial();

		vec3 hitPos = rayPos + rayDir * hitInfo.dist;
		hitInfo.material.albedo = vec3(0.9, 0.9, 0.9);
		hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
		hitInfo.material.specularChance = 0.02f;
		hitInfo.material.specularRoughness = 0.0f;
		hitInfo.material.specularColor = vec3(1.0f, 1.0f, 1.0f) * 0.8f;
		hitInfo.material.IOR = 1.5;
		hitInfo.material.refractionChance = 1.0f;
		hitInfo.material.refractionRoughness = 0.0f;
	}


#endif

	
if (RayGridIntersect(rayPos, rayDir, hitInfo, vec3(data_SSBO[0], data_SSBO[1], data_SSBO[2]), vec3(data_SSBO[3], data_SSBO[4], data_SSBO[5]))) {

	for (int i = 6; i < data_SSBO.length(); i = i + 9) {
		if (testTriangleIntersect(rayPos, rayDir, vec3(data_SSBO[i], data_SSBO[i + 1], data_SSBO[i + 2]), vec3(data_SSBO[i + 3], data_SSBO[i + 4], data_SSBO[i + 5]), vec3(data_SSBO[i + 6], data_SSBO[i + 7], data_SSBO[i + 8]), hitInfo))
		{
			hitInfo.material = GetZeroedMaterial();
			hitInfo.material.albedo = vec3(0.7f, 0.7f, 0.7f);
		}
	}

}
	
/*
	for (int i = 0; i < data_SSBO.length(); i= i + 9) {
		if (testTriangleIntersect(rayPos, rayDir, vec3(data_SSBO[i], data_SSBO[i+1], data_SSBO[i+2]), vec3(data_SSBO[i+3], data_SSBO[i+4], data_SSBO[i+5]), vec3(data_SSBO[i+6], data_SSBO[i+7], data_SSBO[i+8]), hitInfo))
		{
			hitInfo.material = GetZeroedMaterial();
			hitInfo.material.albedo = vec3(0.7f, 0.7f, 0.7f);
		}
	}

	*/
	
	


	// light
	/*
	{
		vec3 A = vec3(-5.0f, 12.4f, 22.5f) + sceneTranslation;
		vec3 B = vec3(5.0f, 12.4f, 22.5f) + sceneTranslation;
		vec3 C = vec3(5.0f, 12.4f, 17.5f) + sceneTranslation;
		vec3 D = vec3(-5.0f, 12.4f, 17.5f) + sceneTranslation;
		if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
		{
			hitInfo.material.albedo = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.emissive = vec3(1.0f, 0.9f, 0.7f);
			hitInfo.material.percentSpecular = 0.0f;
			hitInfo.material.roughness = 0.0f;
			hitInfo.material.specularColor = vec3(0.0f, 0.0f, 0.0f);
		}
	}
	*/

	// back wall
	/*
	{


		vec3 A = vec3(-10.6f, -12.6f, 15.0f) + sceneTranslation;
		vec3 B = vec3(10.6f, -12.6f, 15.0f) + sceneTranslation;
		vec3 C = vec3(10.6f, 12.6f, 15.0f) + sceneTranslation;
		vec3 D = vec3(-10.6f, 12.6f, 15.0f) + sceneTranslation;
		if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
		{
			hitInfo.material.albedo = vec3(0.7f, 0.7f, 0.7f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.percentSpecular = 0.0f;
			hitInfo.material.roughness = 0.5f;
			hitInfo.material.specularColor = vec3(0.0f, 0.0f, 0.0f);
		}
	}
	{


		vec3 A = vec3(-12.6f, -12.6f, 25.0f) + sceneTranslation;
		vec3 B = vec3(12.6f, -12.6f, 25.0f) + sceneTranslation;
		vec3 C = vec3(12.6f, 12.6f, 25.0f) + sceneTranslation;
		vec3 D = vec3(-12.6f, 12.6f, 25.0f) + sceneTranslation;
		if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
		{
			hitInfo.material.albedo = vec3(0.7f, 0.7f, 0.7f);
			hitInfo.material.emissive = vec3(1.0f, 0.8f, 0.6f)*20.0;
			hitInfo.material.percentSpecular = 0.0f;
			hitInfo.material.roughness = 0.5f;
			hitInfo.material.specularColor = vec3(0.0f, 0.0f, 0.0f);
		}
	}
	{
		vec3 A = vec3(-12.6f, -12.6f, 15.0f) + sceneTranslation;
		vec3 B = vec3(-12.6f, 12.6f, 15.0f) + sceneTranslation;
		vec3 C = vec3(-12.6f, 12.6f, 0.0f) + sceneTranslation;
		vec3 D = vec3(-12.6f, -12.6f, 0.0f) + sceneTranslation;
		if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
		{
			hitInfo.material.albedo = vec3(0.7f, 0.7f, 0.7f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.percentSpecular = 0.0f;
			hitInfo.material.roughness = 0.5f;
			hitInfo.material.specularColor = vec3(0.0f, 0.0f, 0.0f);
		}
	}

	{
		vec3 A = vec3(12.6f, -12.6f, 15.0f) + sceneTranslation;
		vec3 B = vec3(12.6f, 12.6f, 15.0f) + sceneTranslation;
		vec3 C = vec3(12.6f, 12.6f, 0.0f) + sceneTranslation;
		vec3 D = vec3(12.6f, -12.6f, 0.0f) + sceneTranslation;
		if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
		{
			hitInfo.material.albedo = vec3(0.7f, 0.7f, 0.7f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.percentSpecular = 0.0f;
			hitInfo.material.roughness = 0.5f;
			hitInfo.material.specularColor = vec3(0.0f, 0.0f, 0.0f);
		}
	}

	{
		vec3 A = vec3(12.6f, -12.6f, 0.0f) + sceneTranslation;
		vec3 B = vec3(12.6f, 12.6f, 0.0f) + sceneTranslation;
		vec3 C = vec3(-12.6f, 12.6f, 0.0f) + sceneTranslation;
		vec3 D = vec3(-12.6f, -12.6f, 0.0f) + sceneTranslation;
		if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
		{
			hitInfo.material.albedo = vec3(0.7f, 0.7f, 0.7f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.percentSpecular = 0.0f;
			hitInfo.material.roughness = 0.5f;
			hitInfo.material.specularColor = vec3(0.0f, 0.0f, 0.0f);
		}
	}
	*/

	/*
	if (testTriangleIntersect(rayPos, rayDir, sphereX, vec3(0.0f, 0.0f, 5.0f), vec3(0.0f, 1.0f, 5.0f), hitInfo))
	{
		hitInfo.material.albedo = vec3(1.0f, 1.0f, 1.0f);
		hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
		hitInfo.material.percentSpecular = 1.0f;
		hitInfo.material.roughness = 0.0f;
		hitInfo.material.specularColor = vec3(1.0f, 1.0f, 1.0f);
	}
	*/
}

vec3 GetColorForRay(in vec3 startRayPos, in vec3 startRayDir, inout uint rngState)
{
	// initialize
	vec3 ret = vec3(0.0f, 0.0f, 0.0f);
	vec3 throughput = vec3(1.0f, 1.0f, 1.0f);
	vec3 rayPos = startRayPos;
	vec3 rayDir = startRayDir;

	for (int bounceIndex = 0; bounceIndex <= 8; ++bounceIndex)
	{
		// shoot a ray out into the world
		SRayHitInfo hitInfo;
		hitInfo.material = GetZeroedMaterial();
		hitInfo.dist = c_superFar;
		hitInfo.fromInside = false;
		TestSceneTrace(rayPos, rayDir, hitInfo);
		bool miss = false;

		// if the ray missed, we are done
		if (hitInfo.dist == c_superFar)
		{	
			ret += min(texture(equirectangularMap, SampleSphericalMap(normalize(rayDir))).rgb, vec3(1.0)) * throughput;
			break;
		}


		// do absorption if we are hitting from inside the object
		if (hitInfo.fromInside)
			throughput *= exp(-hitInfo.material.refractionColor * hitInfo.dist);

		// get the pre-fresnel chances
		float specularChance = hitInfo.material.specularChance;
		float refractionChance = hitInfo.material.refractionChance;

		float diffuseChance = max(0.0f, 1.0f - (refractionChance + specularChance));

		// take fresnel into account for specularChance and adjust other chances.
		// specular takes priority.
		// chanceMultiplier makes sure we keep diffuse / refraction ratio the same.
		float rayProbability = 1.0f;
		if (specularChance > 0.0f)
		{
			specularChance = FresnelReflectAmount(
				hitInfo.fromInside ? hitInfo.material.IOR : 1.0,
				!hitInfo.fromInside ? hitInfo.material.IOR : 1.0,
				rayDir, hitInfo.normal, hitInfo.material.specularChance, 1.0f);

			float chanceMultiplier = (1.0f - specularChance) / (1.0f - hitInfo.material.specularChance);
			refractionChance *= chanceMultiplier;
			diffuseChance *= chanceMultiplier;
		}


		// calculate whether we are going to do a diffuse, specular, or refractive ray
		float doSpecular = 0.0f;
		float doRefraction = 0.0f;
		float raySelectRoll = RandomFloat01(rngState);
		if (specularChance > 0.0f && raySelectRoll < specularChance)
		{
			doSpecular = 1.0f;
			rayProbability = specularChance;
		}
		else if (refractionChance > 0.0f && raySelectRoll < specularChance + refractionChance)
		{
			doRefraction = 1.0f;
			rayProbability = refractionChance;
		}
		else
		{
			rayProbability = 1.0f - (specularChance + refractionChance);
		}

		// numerical problems can cause rayProbability to become small enough to cause a divide by zero.
		rayProbability = max(rayProbability, 0.001f);

		// update the ray position
		if (doRefraction == 1.0f)
		{
			rayPos = (rayPos + rayDir * hitInfo.dist) - hitInfo.normal * c_rayPosNormalNudge;
		}
		else
		{
			rayPos = (rayPos + rayDir * hitInfo.dist) + hitInfo.normal * c_rayPosNormalNudge;
		}

		// Calculate a new ray direction.
		// Diffuse uses a normal oriented cosine weighted hemisphere sample.
		// Perfectly smooth specular uses the reflection ray.
		// Rough (glossy) specular lerps from the smooth specular to the rough diffuse by the material roughness squared
		// Squaring the roughness is just a convention to make roughness feel more linear perceptually.
		vec3 diffuseRayDir = normalize(hitInfo.normal + RandomUnitVector(rngState));

		vec3 specularRayDir = reflect(rayDir, hitInfo.normal);
		specularRayDir = normalize(mix(specularRayDir, diffuseRayDir, hitInfo.material.specularRoughness*hitInfo.material.specularRoughness));

		vec3 refractionRayDir = refract(rayDir, hitInfo.normal, hitInfo.fromInside ? hitInfo.material.IOR : 1.0f / hitInfo.material.IOR);
		refractionRayDir = normalize(mix(refractionRayDir, normalize(-hitInfo.normal + RandomUnitVector(rngState)), hitInfo.material.refractionRoughness*hitInfo.material.refractionRoughness));

		rayDir = mix(diffuseRayDir, specularRayDir, doSpecular);
		rayDir = mix(rayDir, refractionRayDir, doRefraction);

		// add in emissive lighting
		ret += hitInfo.material.emissive * throughput;

		// update the colorMultiplier. refraction doesn't alter the color until we hit the next thing, so we can do light absorption over distance.
		if (doRefraction == 0.0f)
			throughput *= mix(hitInfo.material.albedo, hitInfo.material.specularColor, doSpecular);

		// since we chose randomly between diffuse, specular, refract,
		// we need to account for the times we didn't do one or the other.
		throughput /= rayProbability;

		// Russian Roulette
		// As the throughput gets smaller, the ray is more likely to get terminated early.
		// Survivors have their value boosted to make up for fewer samples being in the average.
		{
			float p = max(throughput.r, max(throughput.g, throughput.b));
			if (RandomFloat01(rngState) > p)
				break;

			// Add the energy we 'lose' by randomly terminating paths
			throughput *= 1.0f / p;
		}

		throughput = clamp(throughput, 0.0, 1.0);

		if (miss) {
			break;
		}

	}

	// return pixel color
	return ret;
}

void GetCameraVectors(out vec3 cameraPos, out vec3 cameraFwd, out vec3 cameraUp, out vec3 cameraRight)
{
	// if the mouse is at (0,0) it hasn't been moved yet, so use a default camera setup
	vec2 mouse = vec2(angleX, angleY);
	if (dot(mouse, vec2(1.0f, 1.0f)) == 0.0f)
	{
		cameraPos = vec3(0.0f, 0.0f, -c_cameraDistance);
		cameraFwd = vec3(0.0f, 0.0f, 1.0f);
		cameraUp = vec3(0.0f, 1.0f, 0.0f);
		cameraRight = vec3(1.0f, 0.0f, 0.0f);
		return;
	}

	// otherwise use the mouse position to calculate camera position and orientation

	cameraFwd.x = cos(angleX) * cos(angleY) * c_cameraDistance;
	cameraFwd.y = -sin(angleY) * c_cameraDistance;
	cameraFwd.z = sin(angleX) * cos(angleY) * c_cameraDistance;
	
	cameraPos = normalize(c_cameraAt - cameraFwd);

	cameraRight = normalize(cross(vec3(0.0f, 1.0f, 0.0f), cameraPos));
	cameraUp = normalize(cross(cameraPos, cameraRight));
}


void main() {
	// base pixel colour for image
	vec4 pixel = vec4(0.0, 0.0, 0.0, 1.0);
	// get index in global work group i.e x,y position
	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
	vec4 texturecolor = imageLoad(img_output, pixel_coords.xy);

	// initialize a random number state based on frag coord and frame
	uint rngState = uint(uint(pixel_coords.x) * uint(1973) + uint(pixel_coords.y) * uint(9277) + uint(iTime) * uint(26699)) | uint(1);

	// calculate subpixel camera jitter for anti aliasing
	vec2 jitter = vec2(RandomFloat01(rngState), RandomFloat01(rngState)) - 0.5f;

	// get the camera vectors
	
	vec3 rayDir;
	{
		// calculate a screen position from -1 to +1 on each axis
		vec2 rt = vec2(pixel_coords.x, pixel_coords.y);
		vec2 uvJittered = vec2((rt + jitter) / vec2(game_window_x, game_window_y));
		vec2 screen = uvJittered * 2.0f - 1.0f;

		// adjust for aspect ratio
		float aspectRatio = game_window_x / game_window_y;
		screen.y /= aspectRatio;

		// make a ray direction based on camera orientation and field of view angle
		float cameraDistance = tan(FOV * 0.5f * c_pi / 180.0f);
		rayDir = vec3(screen, cameraDistance);
		rayDir = normalize(mat3(cameraRight, cameraUp, cameraPos) * rayDir);
	}

	//raytrace for this pxiel
	vec3 color = vec3(0.0f, 0.0f, 0.0f);

	float blend = (iFrame < 2 || texturecolor.a == 0.0f ) ? 1.0f : 1.0f / (1.0f + (1.0f / texturecolor.a));
	for (int index = 0; index < c_numRendersPerFrame; ++index)
		color += GetColorForRay(cameraPos + cameraMov, rayDir, rngState) / float(c_numRendersPerFrame);


	// convert from linear to sRGB for display
	color = mix(texturecolor.rgb, color, blend);

	// output to a specific pixel in the image
	imageStore(img_output, pixel_coords, vec4(color, blend));
}
