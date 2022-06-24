#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "stb_image.h"

#include <vector>
#include <iostream>

class Primitives{
public:

	int Size;
	std::string Type;
	unsigned int VBO;
	unsigned int VAO;
	unsigned int diffuseMap;
	unsigned int specularMap;
	unsigned int normalMap;
	unsigned int heightMap;
	
	
	Primitives(const std::string type, int size) {
		Size = size;
		Type = type;
		std::vector<float> Vertices;
		std::vector<float> NVertices;
		if (type == "cube" || type == "lamp") {
			Vertices = {
				// positions          // normals           // texture coords
				-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
				0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
				0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
				0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
				-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
				-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

				-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
				0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
				0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
				0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
				-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
				-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

				-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
				-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
				-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
				-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
				-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
				-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

				0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
				0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
				0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
				0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
				0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
				0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

				-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
				0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
				0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
				0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
				-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
				-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

				-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
				0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
				0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
				0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
				-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
				-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
			};
		}
		else if (type == "plane") {
			Vertices = {
				// positions          // normals           // texture coords
				-1.5f, -0.5f, -0.5f,  0.0f,  0.0f, 1.0f,  0.0f,  0.0f,
				0.5f, -0.5f, -0.5f,  0.0f,  0.0f, 1.0f,  2.0f,  0.0f,
				0.5f,  0.5f, -0.5f,  0.0f,  0.0f, 1.0f,  2.0f,  1.0f,
				0.5f,  0.5f, -0.5f,  0.0f,  0.0f, 1.0f,  2.0f,  1.0f,
				-1.5f,  0.5f, -0.5f,  0.0f,  0.0f, 1.0f,  0.0f,  1.0f,
				-1.5f, -0.5f, -0.5f,  0.0f,  0.0f, 1.0f,  0.0f,  0.0f,
			};
		}
		else if (type == "floor") {
			Vertices = {
				-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  0.0f,  0.0f,  4.0f,
				0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  0.0f,  4.0f,  4.0f,
				0.5f, -0.5f,  0.5f,  0.0f, 1.0f,  0.0f,  4.0f,  0.0f,
				0.5f, -0.5f,  0.5f,  0.0f, 1.0f,  0.0f,  4.0f,  0.0f,
				-0.5f, -0.5f,  0.5f,  0.0f, 1.0f,  0.0f,  0.0f,  0.0f,
				-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  0.0f,  0.0f,  4.0f,
			};
		}

		//create the tangent, bitangent, and normal matrix for the object
		for (int i = 0; i < Vertices.size(); i += 24) {
			glm::vec3 pos1(Vertices[i + 0], Vertices[i + 1], Vertices[i + 2]);
			glm::vec3 pos2(Vertices[i + 8], Vertices[i + 9], Vertices[i + 10]);
			glm::vec3 pos3(Vertices[i + 16], Vertices[i + 17], Vertices[i + 18]);

			glm::vec2 uv1(Vertices[i + 6], Vertices[i + 7]);
			glm::vec2 uv2(Vertices[i + 14], Vertices[i + 15]);
			glm::vec2 uv3(Vertices[i + 22], Vertices[i + 23]);

			glm::vec3 nm(Vertices[i + 3], Vertices[i + 4], Vertices[i + 5]);

			glm::vec3 edge1 = pos2 - pos1;
			glm::vec3 edge2 = pos3 - pos1;
			glm::vec2 deltaUV1= uv2 - uv1;
			glm::vec2 deltaUV2 = uv3 - uv1;

			// calculate tangent/bitangent vectors of both triangles
			glm::vec3 tangent1, bitangent1;
			glm::vec3 tangent2, bitangent2;

			float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

			bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
			bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
			bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

			NVertices.insert(NVertices.end(), { pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z });
			NVertices.insert(NVertices.end(), { pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z });
			NVertices.insert(NVertices.end(), { pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z });
		}
		

		glGenBuffers(1, &VBO);
		glGenVertexArrays(1, &VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		//cast the std::vector to a const void* for a parameter in this function
		glBufferData(GL_ARRAY_BUFFER, NVertices.size() * sizeof(float), static_cast<const void*>(NVertices.data()), GL_STATIC_DRAW);
	
		glBindVertexArray(VAO);

		// position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		
		// normal attribute
		if (Type != "lamp") {
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
			glEnableVertexAttribArray(4);
		}

		// load textures (we now use a utility function to keep the code more organized)
		// -----------------------------------------------------------------------------
		if (type == "cube") {
			diffuseMap = loadTexture("\create.png");
			specularMap = loadTexture("\container_specular.png");
			normalMap = loadTexture("blank_normal.jpg");
			heightMap = loadTexture("blank_height.png");
		}
		else if (type == "plane") {
			diffuseMap = loadTexture("assets\\textures\\brickwall_diffuse.jpg");
			specularMap = loadTexture("assets\\textures\\sifi_specular.jpg");
			normalMap = loadTexture("assets\\textures\\brickwall_normal.jpg");
			heightMap = loadTexture("assets\\textures\\brickwall_height.png");
		}
		else if (type == "floor") {
			diffuseMap = loadTexture("pebbles_diffuse.jpg");
			specularMap = loadTexture("blank.jpg");
			normalMap = loadTexture("pebbles_normal.jpg");
			heightMap = loadTexture("pebbles_height.png");
		}
	}

	void createVBO() {
		glGenBuffers(1, &VBO);
	}

	void bindAllocateVBO() {
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		//glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
	}

	void createVAO() {
		glGenVertexArrays(1, &VAO);
	}

	void configVAO() {
		glBindVertexArray(VAO);

		// position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		// normal attribute
		if (Type != "lamp") {
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);
		}
	}

	void render() {
		glBindVertexArray(VAO);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);//wire frame
		if (Type != "plane") {
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}
		else {
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		
	}

	// utility function for loading a 2D texture from file
	// ---------------------------------------------------
	unsigned int loadTexture(char const * path)
	{
		unsigned int textureID;
		glGenTextures(1, &textureID);

		int width, height, nrComponents;
		unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
		if (data)
		{
			GLenum format;
			if (nrComponents == 1)
				format = GL_RED;
			else if (nrComponents == 3)
				format = GL_RGB;
			else if (nrComponents == 4)
				format = GL_RGBA;

			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat 
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			stbi_image_free(data);
		}
		else
		{
			std::cout << "Texture failed to load at path: " << path << std::endl;
			stbi_image_free(data);
		}

		return textureID;
	}

};
#endif
