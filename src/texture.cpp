
#include "texture.h"


Texture::Texture() {
	glGenTextures(1, &id);
	activeTexture = 0;
}

void Texture::bind(int textureUnit) {
	glActiveTexture(GL_TEXTURE0+textureUnit);
	activeTexture = textureUnit;
	glBindTexture(GL_TEXTURE_2D, id);

}

void Texture::unbind() {
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::createTexture(float width, float height, GLint internalformat, GLenum format, float *d) {
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, GL_FLOAT, d);
	glBindImageTexture(activeTexture, id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
}

Texture::~Texture() {
	glDeleteTextures(1, &id);
}