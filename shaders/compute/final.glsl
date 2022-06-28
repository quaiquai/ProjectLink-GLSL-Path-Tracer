#version 430
precision highp float;
layout(local_size_x = 8, local_size_y = 8) in;

layout(rgba32f, binding = 0) uniform image2D img_input;
layout(rgba32f, binding = 3) uniform image2D img_output;

uniform int game_window_x;
uniform int game_window_y;


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

void main() {

	// get index in global work group i.e x,y position
	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy  );
	vec3 texturecolor = imageLoad(img_input, (pixel_coords.xy)).rgb;

	texturecolor *= 0.5;

	texturecolor = ACESFilm(texturecolor);

	texturecolor = LinearToSRGB(texturecolor);

	imageStore(img_output, pixel_coords, vec4(texturecolor, 1.0));
}