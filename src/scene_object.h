#pragma once

#include <vector>
#include <string>


class SceneObject {

public:

	std::string objectName;
	std::vector<float> position{ 0.0f, 0.0f, 0.0f };

	SceneObject() = default;

	SceneObject(std::string name, std::vector<float> pos);

};