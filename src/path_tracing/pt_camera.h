#ifndef PT_CAMERA
#define PT_CAMERA

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>


class PTCamera {

public:
	float fov = 50.f;
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 1.0);
	glm::vec3 cameraFwd = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 cameraMov = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 cameraRight = glm::vec3(1.0f, 0.0f, 0.0f);
	

	PTCamera(float f, glm::vec3 position);
};


#endif
