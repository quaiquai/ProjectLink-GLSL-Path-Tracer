#pragma once

#include "path_tracing/pt_camera.h"

namespace gLink {
	const int SCR_WIDTH = 1600;
	const int SCR_HEIGHT = 800;

	inline float lastX = gLink::SCR_WIDTH / 2.0f;
	inline float lastY = gLink::SCR_HEIGHT / 2.0f;

	inline bool firstMouse = true;
	inline bool w_press = false;

	static float deltaTime = 0.0f;	// time between current frame and last frame
	inline float lastFrame = 0.0f;
	inline float heightScale = 0.06;

	inline float game_scene_viewport_sizeX = 960;
	inline float game_scene_viewport_sizeY = 540;

	inline PTCamera camera(50.f, glm::vec3(0.0f, 0.0f, 1.0f));
	inline std::string cursorCallbackMode = "unlocked";

	inline float frame_count;

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	inline void initGLAD() {
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cout << "Failed to initialize GLAD" << std::endl;
			return;
		}
	}

}
