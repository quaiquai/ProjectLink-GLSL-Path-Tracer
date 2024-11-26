#pragma once

#include <glad/glad.h>


class Texture {
public:
	GLuint id;
	int activeTexture;
	float *data;

	Texture();

	void bind(int activeT);
	void unbind();
	void createTexture(float width, float height, GLint internalformat, GLenum format, float* d);

	~Texture();
	

};