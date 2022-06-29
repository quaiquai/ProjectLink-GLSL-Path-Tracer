#version 430
precision highp float;
layout(local_size_x = 8, local_size_y = 8) in;

layout(rgba32f, binding = 0) uniform image2D img_output;

uniform vec3 camera;
uniform float iTime;
uniform vec3 sphereX;
uniform float game_window_x;
uniform float game_window_y;
uniform vec2 iMouse;
uniform float angleX;
uniform float angleY;
uniform bool w_press;
uniform vec3 cameraPos, cameraFwd, cameraUp, cameraRight, cameraMov;

layout(std430, binding = 3) buffer layoutName
{
	float data_SSBO[];
};


// The minimunm distance a ray must travel before we consider an intersection.
// This is to prevent a ray from intersecting a surface it just bounced off of.
const float c_minimumRayHitTime = 0.01;
const float c_superFar = 10000.0;

const float c_pi = 3.14159265359f;
const float c_twopi = 2.0f * c_pi;

// a pixel value multiplier of light before tone mapping and sRGB
const float c_exposure = 0.5f;

// after a hit, it moves the ray this far along the normal away from a surface.
// Helps prevent incorrect intersections when rays bounce off of objects.
const float c_rayPosNormalNudge = 0.001;

// how many renders per frame - to get around the vsync limitation.
const int c_numRendersPerFrame = 2;
const float c_minCameraAngle = 0.01f;
const float c_maxCameraAngle = (c_pi - 0.01f);
vec3 c_cameraAt = camera;
const float c_cameraDistance = 0.1f;


const vec3 light = vec3(0.0, 10.0, 20.0);

struct SMaterialInfo
{
	vec3 albedo;           // the color used for diffuse lighting
	vec3 emissive;         // how much the surface glows
	float percentSpecular; // percentage chance of doing specular instead of diffuse lighting
	float roughness;       // how rough the specular reflections are
	vec3 specularColor;    // the color tint of specular reflections
};

struct SRayHitInfo
{
	float dist;
	vec3 normal;
	SMaterialInfo material;
};


float ScalarTriple(vec3 u, vec3 v, vec3 w)
{
	return dot(cross(u, v), w);
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

void TestSceneTrace(in vec3 rayPos, in vec3 rayDir, inout SRayHitInfo hitInfo)
{

	// to move the scene around, since we can't move the camera yet
	vec3 sceneTranslation = vec3(0.0f, 0.0f, 5.0f);
	vec4 sceneTranslation4 = vec4(sceneTranslation, 0.0f);

	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 5; j++) {
			for (int k = 0; k < 5; k++) {
				if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(i * 6 - 12, -9.5f + k * 6 - 6, 20.0f + j * 6, 3.0f) + sceneTranslation4))
				{
					hitInfo.material.albedo = vec3(0.9f, 0.5f, 0.9f);
					hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
					hitInfo.material.percentSpecular = 0.3f;
					hitInfo.material.roughness = 0.2;
					hitInfo.material.specularColor = vec3(0.9f, 0.9f, 0.9f);
				}
			}
			
		}
	}

	// floor
	{
		vec3 A = vec3(-12.6f, -12.45f, 45.0f) + sceneTranslation;
		vec3 B = vec3(12.6f, -12.45f, 45.0f) + sceneTranslation;
		vec3 C = vec3(12.6f, -12.45f, 15.0f) + sceneTranslation;
		vec3 D = vec3(-12.6f, -12.45f, 15.0f) + sceneTranslation;
		if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
		{
			hitInfo.material.albedo = vec3(0.2f, 0.7f, 0.7f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.percentSpecular = 0.0f;
			hitInfo.material.roughness = 0.0f;
			hitInfo.material.specularColor = vec3(0.0f, 0.0f, 0.0f);
		}

		
	}

	// light
	{
		vec3 A = vec3(-5.0f, 12.4f, 22.5f) + sceneTranslation;
		vec3 B = vec3(5.0f, 12.4f, 22.5f) + sceneTranslation;
		vec3 C = vec3(5.0f, 12.4f, 17.5f) + sceneTranslation;
		vec3 D = vec3(-5.0f, 12.4f, 17.5f) + sceneTranslation;
		if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
		{
			hitInfo.material.albedo = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.emissive = vec3(1.0f, 0.9f, 0.7f) * 50f;
			hitInfo.material.percentSpecular = 0.0f;
			hitInfo.material.roughness = 0.0f;
			hitInfo.material.specularColor = vec3(0.0f, 0.0f, 0.0f);
		}
	}

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

float shadow(in vec3 rayPos, in vec3 rayDir, inout SRayHitInfo hitInfo) {
	// to move the scene around, since we can't move the camera yet
	vec3 sceneTranslation = vec3(0.0f, 0.0f, 5.0f);
	vec4 sceneTranslation4 = vec4(sceneTranslation, 0.0f);

	float mag = length(rayDir);

	float att = 0.2 + (1.0 / (1.0 + 2.0*mag + 2.0*mag*mag));

	for (int i = 0; i < data_SSBO.length(); i += 3) {
		if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(data_SSBO[i], data_SSBO[i + 1], data_SSBO[i + 2], 3.0f) + sceneTranslation4))
		{
			return att;
		}

	}

	// back wall
	{


		vec3 A = vec3(-12.6f, -12.6f, 25.0f) + sceneTranslation;
		vec3 B = vec3(12.6f, -12.6f, 25.0f) + sceneTranslation;
		vec3 C = vec3(12.6f, 12.6f, 25.0f) + sceneTranslation;
		vec3 D = vec3(-12.6f, 12.6f, 25.0f) + sceneTranslation;
		if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
		{
			return att;
		}


	}

	// floor
	{
		vec3 A = vec3(-12.6f, -12.45f, 25.0f) + sceneTranslation;
		vec3 B = vec3(12.6f, -12.45f, 25.0f) + sceneTranslation;
		vec3 C = vec3(12.6f, -12.45f, 15.0f) + sceneTranslation;
		vec3 D = vec3(-12.6f, -12.45f, 15.0f) + sceneTranslation;
		if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
		{
			return att;
		}
	}

	// cieling
	{
		vec3 A = vec3(-12.6f, 12.5f, 25.0f) + sceneTranslation;
		vec3 B = vec3(12.6f, 12.5f, 25.0f) + sceneTranslation;
		vec3 C = vec3(12.6f, 12.5f, 15.0f) + sceneTranslation;
		vec3 D = vec3(-12.6f, 12.5f, 15.0f) + sceneTranslation;
		if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
		{
			return att;
		}
	}

	// left wall
	{
		vec3 A = vec3(-12.5f, -12.6f, 25.0f) + sceneTranslation;
		vec3 B = vec3(-12.5f, -12.6f, 15.0f) + sceneTranslation;
		vec3 C = vec3(-12.5f, 12.6f, 15.0f) + sceneTranslation;
		vec3 D = vec3(-12.5f, 12.6f, 25.0f) + sceneTranslation;
		if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
		{
			return att;
		}
	}

	// right wall 
	{
		vec3 A = vec3(12.5f, -12.6f, 25.0f) + sceneTranslation;
		vec3 B = vec3(12.5f, -12.6f, 15.0f) + sceneTranslation;
		vec3 C = vec3(12.5f, 12.6f, 15.0f) + sceneTranslation;
		vec3 D = vec3(12.5f, 12.6f, 25.0f) + sceneTranslation;
		if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
		{
			return att;
		}
	}

	// light
	{
		vec3 A = vec3(-5.0f, 12.4f, 22.5f) + sceneTranslation;
		vec3 B = vec3(5.0f, 12.4f, 22.5f) + sceneTranslation;
		vec3 C = vec3(5.0f, 12.4f, 17.5f) + sceneTranslation;
		vec3 D = vec3(-5.0f, 12.4f, 17.5f) + sceneTranslation;
		if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
		{
			return att;
		}
	}

	if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(-9.0f, -9.5f, 20.0f, 3.0f) + sceneTranslation4))
	{
		return att;
	}

	if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(0.0f, -9.5f, 20.0f, 3.0f) + sceneTranslation4))
	{
		return att;
	}

	// a ball which has blue diffuse but red specular. an example of a "bad material".
	// a better lighting model wouldn't let you do this sort of thing
	if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(9.0f, -9.5f, 20.0f, 3.0f) + sceneTranslation4))
	{
		return att;
	}

	if (testTriangleIntersect(rayPos, rayDir, sphereX, vec3(0.0f, 0.0f, 5.0f), vec3(0.0f, 1.0f, 5.0f), hitInfo))
	{
		return att;
	}

	// shiny green balls of varying roughnesses
	{
		if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(-10.0f, 0.0f, 23.0f, 1.75f) + sceneTranslation4))
		{
			return att;
		}

		if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(-5.0f, 0.0f, 23.0f, 1.75f) + sceneTranslation4))
		{
			return att;
		}

		if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(0.0f, 0.0f, 23.0f, 1.75f) + sceneTranslation4))
		{
			return att;
		}

		if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(5.0f, 0.0f, 23.0f, 1.75f) + sceneTranslation4))
		{
			return att;
		}

		if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(10.0f, 0.0f, 23.0f, 1.75f) + sceneTranslation4))
		{
			return att;
		}

		return 1.0;
	}
}

vec3 GetColorForRay(in vec3 startRayPos, in vec3 startRayDir, inout uint rngState)
{
	// initialize
	vec3 ret = vec3(0.0f, 0.0f, 0.0f);
	vec3 throughput = vec3(1.0f, 1.0f, 1.0f);
	vec3 rayPos = startRayPos;
	vec3 rayDir = startRayDir;

	for (int bounceIndex = 0; bounceIndex <= 5; ++bounceIndex)
	{
		// shoot a ray out into the world
		SRayHitInfo hitInfo;
		hitInfo.dist = c_superFar;
		TestSceneTrace(rayPos, rayDir, hitInfo);

		// if the ray missed, we are done
		if (hitInfo.dist == c_superFar)
		{
			ret += vec3(0.1, 0.1, 0.1) * 2.0 * throughput;
			break;
		}

		// update the ray position
		rayPos = (rayPos + rayDir * hitInfo.dist) + hitInfo.normal * c_rayPosNormalNudge;

		// calculate whether we are going to do a diffuse or specular reflection ray 
		float doSpecular = (RandomFloat01(rngState) < hitInfo.material.percentSpecular) ? 1.0f : 0.0f;

		// Calculate a new ray direction.
		// Diffuse uses a normal oriented cosine weighted hemisphere sample.
		// Perfectly smooth specular uses the reflection ray.
		// Rough (glossy) specular lerps from the smooth specular to the rough diffuse by the material roughness squared
		// Squaring the roughness is just a convention to make roughness feel more linear perceptually.
		vec3 diffuseRayDir = normalize(hitInfo.normal + RandomUnitVector(rngState));
		vec3 specularRayDir = reflect(rayDir, hitInfo.normal);
		specularRayDir = normalize(mix(specularRayDir, diffuseRayDir, hitInfo.material.roughness * hitInfo.material.roughness));
		rayDir = mix(diffuseRayDir, specularRayDir, doSpecular);

		//hitInfo.dist = c_superFar;

		float lightIntensity = shadow(rayPos + rayDir, normalize(light - rayPos), hitInfo);

		// add in emissive lighting
		ret += hitInfo.material.emissive * throughput;

		//hitInfo.material.albedo *= lightIntensity;
		//hitInfo.material.specularColor *= lightIntensity;

		// update the colorMultiplier
		//throughput *= mix(hitInfo.material.albedo * max(0.5, dot(normalize(light - rayPos), hitInfo.normal)) * 1.5, hitInfo.material.specularColor, doSpecular);
		throughput *= mix(hitInfo.material.albedo, hitInfo.material.specularColor, doSpecular);
		
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

	if (w_press) {
		cameraPos -= cameraFwd * 0.1;
	}
	
	cameraPos = normalize(c_cameraAt - cameraFwd);

	cameraRight = normalize(cross(vec3(0.0f, 1.0f, 0.0f), cameraPos));
	cameraUp = normalize(cross(cameraPos, cameraRight));
}


void main() {
	// base pixel colour for image
	vec4 pixel = vec4(0.0, 1.0, 0.0, 1.0);
	// get index in global work group i.e x,y position
	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
	vec3 texturecolor = imageLoad(img_output, pixel_coords.xy).rgb;

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
		float cameraDistance = tan(90f * 0.5f * c_pi / 180.0f);
		rayDir = vec3(screen, cameraDistance);
		rayDir = normalize(mat3(cameraRight, cameraUp, cameraPos) * rayDir);
	}


	//raytrace for this pxiel
	vec3 color = vec3(0.0f, 0.0f, 0.0f);

	for (int index = 0; index < c_numRendersPerFrame; ++index)
		color += GetColorForRay(cameraFwd + cameraMov, rayDir, rngState) / float(c_numRendersPerFrame);
	
	// convert from linear to sRGB for display
	color = mix(texturecolor, color, 1.0 / float(iTime + 1.0));

	// output to a specific pixel in the image
	imageStore(img_output, pixel_coords, vec4(color, 1.0));
}
