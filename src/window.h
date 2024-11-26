#pragma once
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <iostream>

#include "globals.h"


class Window {


public:
	GLFWwindow * window;
	const int width;
	const int height;
	std::string name;

	Window() = default;
	Window(const int width, const int height, std::string name);

	void initWindow();	
	void disableWindowCursor();
	bool checkWindowClose();
	float getWindowTime();
	void pollEvents();

	static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	static void processInput(GLFWwindow *window, float* frame_count);
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	static void unlocked_mouse_callback(GLFWwindow* window, double xpos, double ypos);


};