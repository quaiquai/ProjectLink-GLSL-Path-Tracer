
#include "window.h"

Window::Window(const int height, const int width, std::string name) : height{ height }, width{ width }, name{ name } {
	initWindow();
}

void Window::initWindow() {
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

	// glfw window creation
	// --------------------
	window = glfwCreateWindow(gLink::SCR_WIDTH, gLink::SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
	}

	//callback functions for interactions with window
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, unlocked_mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
}

void Window::disableWindowCursor() {
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

bool Window::checkWindowClose() {
	return glfwWindowShouldClose(window);
}

float Window::getWindowTime() {
	return glfwGetTime();
}

void Window::pollEvents() {
	glfwPollEvents();
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void Window::processInput(GLFWwindow *window, float* frame_count)
{
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		*frame_count = 0.0;
		gLink::w_press = true;
		gLink::camera.cameraMov += gLink::camera.cameraFwd * glm::vec3(0.1);
		//camera.ProcessKeyboard(FORWARD, deltaTime);
		std::cout << "w Pressed";
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		*frame_count = 0.0;
		gLink::w_press = true;
		gLink::camera.cameraMov -= gLink::camera.cameraFwd * glm::vec3(0.1);
		//camera.ProcessKeyboard(BACKWARD, deltaTime);
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		*frame_count = 0.0;
		gLink::w_press = true;
		gLink::camera.cameraMov -= gLink::camera.cameraRight * glm::vec3(0.1);
		//camera.ProcessKeyboard(LEFT, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		*frame_count = 0.0;
		gLink::w_press = true;
		gLink::camera.cameraMov += gLink::camera.cameraRight * glm::vec3(0.1);
		//camera.ProcessKeyboard(RIGHT, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		*frame_count = 0.0;
		gLink::w_press = true;
		gLink::camera.cameraMov.y -= 0.1;
		//camera.ProcessKeyboard(DOWN, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		*frame_count = 0.0;
		gLink::w_press = true;
		gLink::camera.cameraMov.y += 0.1;
		//camera.ProcessKeyboard(UP, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
		if (gLink::cursorCallbackMode == "locked") {
			//glfwSetWindowShouldClose(window, true); //closes window on escape
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			glfwSetCursorPosCallback(window, unlocked_mouse_callback);
			gLink::cursorCallbackMode = "unlocked";
		}
	}
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
		if (gLink::cursorCallbackMode == "unlocked") {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			glfwSetCursorPosCallback(window, mouse_callback);
			gLink::cursorCallbackMode = "locked";
			gLink::firstMouse = true;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true); //closes window on escape
	}

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void Window::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void Window::mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	gLink::frame_count = 0.0;
	if (gLink::firstMouse)
	{
		gLink::lastX = xpos;
		gLink::lastY = ypos;
		gLink::firstMouse = false;
	}

	float xoffset = xpos - gLink::lastX;
	float yoffset = gLink::lastY - ypos; // reversed since y-coordinates go from bottom to top

										 //cout << xoffset;


	gLink::lastX = xpos;
	gLink::lastY = ypos;

	//camera.ProcessMouseMovement(xoffset, yoffset);
}

void Window::unlocked_mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	//cout << xpos << ypos << endl;
}

void Window::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		std::cout << "Cursor Position at (" << xpos << " : " << ypos << std::endl;
	}
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void Window::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	//camera.ProcessMouseScroll(yoffset);
}