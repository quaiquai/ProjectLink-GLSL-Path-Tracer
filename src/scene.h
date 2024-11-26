#pragma once

#include "camera.h"


class Scene {

public:

	Camera* sceneCamera;
	std::vector<SceneObjects> objects;



};