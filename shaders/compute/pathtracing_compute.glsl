#version 430
precision highp float;
layout(local_size_x = 8, local_size_y = 8) in;

layout(rgba32f, binding = 0) uniform image2D img_output;

uniform float iTime;
uniform vec3 sphereX;
uniform float game_window_x;
uniform float game_window_y;
layout(rgba32f, binding = 1) uniform image2D texture;

// The minimunm distance a ray must travel before we consider an intersection.
// This is to prevent a ray from intersecting a surface it just bounced off of.
const float c_minimumRayHitTime = 0.01;
const float c_superFar = 10000.0;

// a pixel value multiplier of light before tone mapping and sRGB
const float c_exposure = 0.8f;

// after a hit, it moves the ray this far along the normal away from a surface.
// Helps prevent incorrect intersections when rays bounce off of objects.
const float c_rayPosNormalNudge = 0.001;

// how many renders per frame - to get around the vsync limitation.
const int c_numRendersPerFrame = 1;

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


vec3 LessThan(vec3 f, float value)
{
	return vec3(
		(f.x < value) ? 1.0f : 0.0f,
		(f.y < value) ? 1.0f : 0.0f,
		(f.z < value) ? 1.0f : 0.0f);
}

vec3 LinearToSRGB(vec3 rgb)
{
	rgb = clamp(rgb, 0.0f, 1.0f);

	return mix(
		pow(rgb, vec3(1.0f / 2.4f)) * 1.055f - 0.055f,
		rgb * 12.92f,
		LessThan(rgb, 0.0031308f)
	);
}

vec3 SRGBToLinear(vec3 rgb)
{
	rgb = clamp(rgb, 0.0f, 1.0f);

	return mix(
		pow(((rgb + 0.055f) / 1.055f), vec3(2.4f)),
		rgb / 12.92f,
		LessThan(rgb, 0.04045f)
	);
}

// ACES tone mapping curve fit to go from HDR to LDR
// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilm(vec3 x)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return clamp((x*(a*x + b)) / (x*(c*x + d) + e), 0.0f, 1.0f);
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
	vec3 sceneTranslation = vec3(0.0f, 0.0f, 10.0f);
	vec4 sceneTranslation4 = vec4(sceneTranslation, 0.0f);

	// back wall
	{

		
		vec3 A = vec3(-12.6f, -12.6f, 25.0f) + sceneTranslation;
		vec3 B = vec3(12.6f, -12.6f, 25.0f) + sceneTranslation;
		vec3 C = vec3(12.6f, 12.6f, 25.0f) + sceneTranslation;
		vec3 D = vec3(-12.6f, 12.6f, 25.0f) + sceneTranslation;
		if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
		{
			hitInfo.material.albedo = vec3(0.7f, 0.7f, 0.7f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.percentSpecular = 0.0f;
			hitInfo.material.roughness = 0.0f;
			hitInfo.material.specularColor = vec3(0.0f, 0.0f, 0.0f);
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
			hitInfo.material.albedo = vec3(0.7f, 0.7f, 0.7f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.percentSpecular = 0.0f;
			hitInfo.material.roughness = 0.0f;
			hitInfo.material.specularColor = vec3(0.0f, 0.0f, 0.0f);
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
			hitInfo.material.albedo = vec3(0.7f, 0.7f, 0.7f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.percentSpecular = 0.0f;
			hitInfo.material.roughness = 0.0f;
			hitInfo.material.specularColor = vec3(0.0f, 0.0f, 0.0f);
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
			hitInfo.material.albedo = vec3(0.7f, 0.1f, 0.1f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.percentSpecular = 0.0f;
			hitInfo.material.roughness = 0.0f;
			hitInfo.material.specularColor = vec3(0.0f, 0.0f, 0.0f);
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
			hitInfo.material.albedo = vec3(0.1f, 0.7f, 0.1f);
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
			hitInfo.material.emissive = vec3(1.0f, 0.9f, 0.7f) * 20.0f;
			hitInfo.material.percentSpecular = 0.0f;
			hitInfo.material.roughness = 0.0f;
			hitInfo.material.specularColor = vec3(0.0f, 0.0f, 0.0f);
		}
	}

	if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(-9.0f, -9.5f, 20.0f, 3.0f) + sceneTranslation4))
	{
		hitInfo.material.albedo = vec3(0.9f, 0.9f, 0.5f);
		hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
		hitInfo.material.percentSpecular = 0.1f;
		hitInfo.material.roughness = 0.2f;
		hitInfo.material.specularColor = vec3(0.9f, 0.9f, 0.9f);
	}

	if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(0.0f, -9.5f, 20.0f, 3.0f) + sceneTranslation4))
	{
		hitInfo.material.albedo = vec3(0.9f, 0.5f, 0.9f);
		hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
		hitInfo.material.percentSpecular = 0.3f;
		hitInfo.material.roughness = 0.2;
		hitInfo.material.specularColor = vec3(0.9f, 0.9f, 0.9f);
	}

	// a ball which has blue diffuse but red specular. an example of a "bad material".
	// a better lighting model wouldn't let you do this sort of thing
	if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(9.0f, -9.5f, 20.0f, 3.0f) + sceneTranslation4))
	{
		hitInfo.material.albedo = vec3(0.0f, 0.0f, 1.0f);
		hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
		hitInfo.material.percentSpecular = 0.5f;
		hitInfo.material.roughness = 0.4f;
		hitInfo.material.specularColor = vec3(1.0f, 0.0f, 0.0f);
	}

	if (testTriangleIntersect(rayPos, rayDir, sphereX, vec3(0.0f, 0.0f, 5.0f), vec3(0.0f, 1.0f, 5.0f), hitInfo))
	{
		hitInfo.material.albedo = vec3(1.0f, 1.0f, 1.0f);
		hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
		hitInfo.material.percentSpecular = 1.0f;
		hitInfo.material.roughness = 0.0f;
		hitInfo.material.specularColor = vec3(1.0f, 1.0f, 1.0f);
	}

	// shiny green balls of varying roughnesses
	{
		if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(-10.0f, 0.0f, 23.0f, 1.75f) + sceneTranslation4))
		{
			hitInfo.material.albedo = vec3(1.0f, 1.0f, 1.0f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.percentSpecular = 1.0f;
			hitInfo.material.roughness = 0.0f;
			hitInfo.material.specularColor = vec3(0.3f, 1.0f, 0.3f);
		}

		if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(-5.0f, 0.0f, 23.0f, 1.75f) + sceneTranslation4))
		{
			hitInfo.material.albedo = vec3(1.0f, 1.0f, 1.0f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.percentSpecular = 1.0f;
			hitInfo.material.roughness = 0.25f;
			hitInfo.material.specularColor = vec3(0.3f, 1.0f, 0.3f);
		}

		if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(0.0f, 0.0f, 23.0f, 1.75f) + sceneTranslation4))
		{
			hitInfo.material.albedo = vec3(1.0f, 1.0f, 1.0f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.percentSpecular = 1.0f;
			hitInfo.material.roughness = 0.5f;
			hitInfo.material.specularColor = vec3(0.3f, 1.0f, 0.3f);
		}

		if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(5.0f, 0.0f, 23.0f, 1.75f) + sceneTranslation4))
		{
			hitInfo.material.albedo = vec3(1.0f, 1.0f, 1.0f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.percentSpecular = 1.0f;
			hitInfo.material.roughness = 0.75f;
			hitInfo.material.specularColor = vec3(0.3f, 1.0f, 0.3f);
		}

		if (TestSphereTrace(rayPos, rayDir, hitInfo, vec4(10.0f, 0.0f, 23.0f, 1.75f) + sceneTranslation4))
		{
			hitInfo.material.albedo = vec3(1.0f, 1.0f, 1.0f);
			hitInfo.material.emissive = vec3(0.0f, 0.0f, 0.0f);
			hitInfo.material.percentSpecular = 1.0f;
			hitInfo.material.roughness = 1.0f;
			hitInfo.material.specularColor = vec3(0.3f, 1.0f, 0.3f);
		}
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
			ret += vec3(0.3, 0.3, 0.1) * 2.0 * throughput;
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

		// add in emissive lighting
		ret += hitInfo.material.emissive * throughput;

		// update the colorMultiplier
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


void main() {
	// base pixel colour for image
	vec4 pixel = vec4(0.0, 1.0, 0.0, 1.0);
	// get index in global work group i.e x,y position
	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
	vec3 texturecolor = imageLoad(texture, pixel_coords.xy).rgb;

	// initialize a random number state based on frag coord and frame
	uint rngState = uint(uint(pixel_coords.x) * uint(1973) + uint(pixel_coords.y) * uint(9277) + uint(iTime) * uint(26699)) | uint(1);

	// The ray starts at the camera position (the origin)
	vec3 rayPosition = vec3(0.0, 0.0, 0.0);

	// calculate the camera distance
	float cameraDistance = 1.0 / tan(90.0 * 0.5 * 3.14159265359f / 180.0);

	// calculate subpixel camera jitter for anti aliasing
	vec2 jitter = vec2(RandomFloat01(rngState), RandomFloat01(rngState)) - 0.5f;

	// calculate coordinates of the ray target on the imaginary pixel plane.
	// -1 to +1 on x,y axis. 1 unit away on the z axis
	vec2 rt = vec2(pixel_coords.x - game_window_x*1.5, pixel_coords.y - game_window_y);
	vec3 rayTarget = vec3(((rt + jitter) / vec2(game_window_x, game_window_y)), cameraDistance);

	// correct for aspect ratio
	float aspectRatio = game_window_x / game_window_y;
	rayTarget.y /= aspectRatio;

	// calculate a normalized vector for the ray direction.
	// it's pointing from the ray position to the ray target.
	vec3 rayDir = normalize(rayTarget - rayPosition);

	//raytrace for this pxiel
	vec3 color = vec3(0.0f, 0.0f, 0.0f);

	for (int index = 0; index < c_numRendersPerFrame; ++index)
		color += GetColorForRay(rayPosition, rayDir, rngState) / float(c_numRendersPerFrame);

	// apply exposure (how long the shutter is open)
	color *= c_exposure;

	// convert unbounded HDR color range to SDR color range
	color = ACESFilm(color);

	// convert from linear to sRGB for display
	color = LinearToSRGB(color);

	color = mix(texturecolor, color, 1.0 / float(iTime + 1.0));

	// output to a specific pixel in the image
	imageStore(img_output, pixel_coords, vec4(color, 1.0));
}
