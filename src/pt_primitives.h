#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

namespace PT {

	struct Material {
		glm::vec3 albedo;
		glm::vec3 emissive;
		float percentSpecular;
		float roughness;
		glm::vec3 specularColor;
	};

	class Primitive {
	public:

		float size;
		glm::vec3 position;
		Material mat;


		Primitive(glm::vec3 pos, float sz, glm::vec3 color, glm::vec3 emissive, glm::vec3 specularColor, float rough, float percentSpec) {
			size = sz;
			position = pos;
			mat.albedo = color;
			mat.emissive = emissive;
			mat.percentSpecular = percentSpec;
			mat.roughness = rough;
			mat.specularColor = specularColor;
		}
	};


	class Sphere: public Primitive {
	public:

		float radius;

		Sphere(glm::vec3 pos, float sz, glm::vec3 color, glm::vec3 emissive, glm::vec3 specularColor, float rough, float percentSpec) 
			: Primitive(pos, sz, color, emissive, specularColor, rough, percentSpec) {

			radius = 1.0;
		}
	};

	class Flat : Primitive {
	public:

		std::string type;
		std::vector<glm::vec3> vertices;

		Flat(std::string tp, glm::vec3 pos, float sz, glm::vec3 color, glm::vec3 emissive, glm::vec3 specularColor, float rough, float percentSpec)
			: Primitive(pos, sz, color, emissive, specularColor, rough, percentSpec) {

			type = tp;

			if (type.compare("floor") == 0) {
				vertices.push_back(glm::vec3(-12.6f, -12.45f, 25.0f));
				vertices.push_back(glm::vec3(12.6f, -12.45f, 25.0f));
				vertices.push_back(glm::vec3(12.6f, -12.45f, 15.0f));
				vertices.push_back(glm::vec3(-12.6f, -12.45f, 15.0f));
			}else if (type.compare("wall") == 0){
				vertices.push_back(glm::vec3(-12.5f, -12.6f, 25.0f));
				vertices.push_back(glm::vec3(-12.5f, -12.6f, 15.0f));
				vertices.push_back(glm::vec3(-12.5f, 12.6f, 15.0f));
				vertices.push_back(glm::vec3(-12.5f, 12.6f, 25.0f));
			}
			
		}
	};

}

