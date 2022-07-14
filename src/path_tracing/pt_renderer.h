#include <GLFW/glfw3.h>

namespace PT {
	class Renderer {
	public:

		unsigned int scrWidth;
		unsigned int scrHeight;


		Renderer(unsigned int w, unsigned int h) {
			scrWidth = w;
			scrHeight = h;

			glfwInit();
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

			// glfw window creation
			// --------------------
			GLFWwindow* window = glfwCreateWindow(scrWidth, scrHeight, "LearnOpenGL", NULL, NULL);
			if (window == NULL)
			{
				std::cout << "Failed to create GLFW window" << std::endl;
				glfwTerminate();
				return -1;
			}
		}

	};


}