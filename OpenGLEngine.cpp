// OpenGLEngine.cpp : Defines the entry point for the console application.
//


#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <iterator>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"




#include "shader.h"
#include "camera.h"
#include "primitives.h"
#include "ui.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
using namespace std;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void unlocked_mouse_callback(GLFWwindow* window, double xpos, double ypos);


// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
string cursorCallbackMode = "locked";

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;
float heightScale = 0.06;

float game_scene_viewport_sizeX = 960;
float game_scene_viewport_sizeY = 540;

//lighting Position
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

unsigned int VBO;
unsigned int VAO;

float sphereX = 0.0;

std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;
std::vector< glm::vec3 > temp_vertices;
std::vector< glm::vec2 > temp_uvs;
std::vector< glm::vec3 > temp_normals;

int main()
{
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
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	//callback functions for interactions with window
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// tell GLFW to capture our mouse
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile our shader zprogram
	// ------------------------------------
	//Shader lightingShader("shaders\\multiple_lights.vs", "shaders\\multiple_lights.fs");
	//Shader lampShader("shaders\\lamp.vs", "shaders\\lamp.fs");
	//Shader singleLightShader("shaders\\single_light_normal.vs", "shaders\\single_light_normal.fs");
	Shader screenShader("shaders\\framebuffer_screen.vs", "shaders\\framebuffer_screen.fs");
	Shader pathtracingShader("shaders\\pathtracing_main.vs", "shaders\\pathtracing_main.fs");
	Shader pathtracingCompute("shaders\\compute\\pathtracing_compute.glsl");

	UI user_interface(window);

	pathtracingShader.use();


	std::string inputfile = "assets//objects//teapot.obj";
	tinyobj::ObjReaderConfig reader_config;
	reader_config.mtl_search_path = "./"; // Path to material files

	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(inputfile, reader_config)) {
		if (!reader.Error().empty()) {
			std::cerr << "TinyObjReader: " << reader.Error();
		}
		exit(1);
	}

	if (!reader.Warning().empty()) {
		std::cout << "TinyObjReader: " << reader.Warning();
	}

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
				tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
				tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

				// Check if `normal_index` is zero or positive. negative = no normal data
				if (idx.normal_index >= 0) {
					tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
					tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
					tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
				}

				// Check if `texcoord_index` is zero or positive. negative = no texcoord data
				if (idx.texcoord_index >= 0) {
					tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
					tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
				}

				// Optional: vertex colors
				// tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
				// tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
				// tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];
			}
			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];
		}
	}




	float vertices[] = { 
		-1.0f, -1.0f,
		-1.0f, 1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f,
		-1.0f, 1.0f,
		1.0f, -1.0f
	};

	int bufferSize = 0;

	for (const auto& vert : attrib.vertices) {
		bufferSize += 4;
		//cout << vert;
	}

	GLuint ssbo;
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, &attrib.vertices, GL_STATIC_READ); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind

	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//cast the std::vector to a const void* for a parameter in this function
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

	glBindVertexArray(VAO);

	// position attribute
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);


	//static variables used in imgui settings
	//translation position for the lamp cube representing the light
	static float translation[] = { 1.2f, 1.0f, 20.0f };
	//scale of the outer angle for the flashlight
	static float outerCutoff = 17.0;
	//the color of the diffuse component of the flashlight
	static float color[4] = { 0.7f,0.7f,0.7f,1.0f };

	pathtracingCompute.use();
	// dimensions of the image
	int tex_w = 512, tex_h = 512;
	unsigned int tex_output;
	glGenTextures(1, &tex_output);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_output);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, game_scene_viewport_sizeX, game_scene_viewport_sizeY, 0, GL_RGBA, GL_FLOAT,
		NULL);

	unsigned int tex_output2;
	glGenTextures(1, &tex_output2);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, tex_output2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, game_scene_viewport_sizeX, game_scene_viewport_sizeY, 0, GL_RGBA, GL_FLOAT,
		NULL);

	/*
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, tex_output[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT,
		NULL);
	*/

	unsigned int textures[2] = { tex_output, tex_output2 };
	glBindImageTexture(0, textures[0], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(1, textures[1], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	

	pathtracingShader.use();


	unsigned int framebuffer;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	unsigned int textureColorbuffer[2];
	glGenTextures(2, textureColorbuffer);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, game_scene_viewport_sizeX, game_scene_viewport_sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, textureColorbuffer[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, game_scene_viewport_sizeX, game_scene_viewport_sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer[0], 0);

	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, game_scene_viewport_sizeX, game_scene_viewport_sizeY); // use a single renderbuffer object for both a depth AND stencil buffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;

	float frame_count = 0.0;
	pathtracingCompute.use();
	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		frame_count++;

		// input
		// -----
		
		glfwPollEvents();

		// feed inputs to dear imgui, start new frame
		user_interface.newFrameUI();

		

		{ // launch compute shaders!
			pathtracingCompute.setVec3("sphereX", glm::vec3(translation[0], translation[1], translation[2]));
			pathtracingCompute.setFloat("game_window_x", tex_w);
			pathtracingCompute.setFloat("game_window_y", tex_h);
			pathtracingCompute.setFloat("iTime", frame_count);
			glDispatchCompute(((GLuint)game_scene_viewport_sizeX + 8 - 1)/8, ((GLuint)game_scene_viewport_sizeY + 8 - 1) / 8, 1);
		}

		

		// make sure writing to image has finished before read
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//reverse textures for pingpong rendering
		std::reverse(begin(textures), end(textures));
		glBindImageTexture(0, textures[0], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(1, textures[1], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		/*
		Here we have to rebind the framebuffer in the beginning to render to the texture bound to it.
		We rebind the texture and render buffer to change its size dependent on the size of the imgui window its rendered in.
		*/
		/*
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer[0], 0);

		pathtracingShader.setInt("texture", 0);
		pathtracingShader.setVec3("sphereX", glm::vec3(translation[0], translation[1], translation[2]));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer[1]);

		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, game_scene_viewport_sizeX, game_scene_viewport_sizeY); // use a single renderbuffer object for both a depth AND stencil buffer.
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it

		pathtracingShader.setFloat("game_window_x", game_scene_viewport_sizeX);
		pathtracingShader.setFloat("game_window_y", game_scene_viewport_sizeY);
		pathtracingShader.setFloat("iTime", frame_count);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		//cast the std::vector to a const void* for a parameter in this function
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

		glBindVertexArray(VAO);

		// position attribute
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(0);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//reverse textures for pingpong rendering
		std::reverse(begin(textureColorbuffer), end(textureColorbuffer));
		*/
		
		
		ImGui::Begin("Triangle Position/Color");
		
		if (ImGui::SliderFloat3("sphereX", translation, -1.0, 50.0)) {
			frame_count = 0.0;
		}
		
		// color picker
		ImGui::ColorEdit3("color", color);
		ImGui::End();

		//user_interface.drawMenus();
		

		
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glDisable(GL_DEPTH_TEST);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		/*
		Begins the code that adds rendered texture to a imgui  window to be seen. 
		Invisible button is added to determine if a cursor is within the bounds of the window
		to track mouse events only for the texture so we can do camera dragging and mouse picking events.
		*/

		ImGui::Begin("GameWindow");
		{
			ImVec2 window_size = ImGui::GetContentRegionAvail();

			if (window_size.x != game_scene_viewport_sizeX && window_size.y != game_scene_viewport_sizeY) {
				//resize has happened or first inital render
				game_scene_viewport_sizeX = window_size.x;
				game_scene_viewport_sizeY = window_size.y;

				//resize the textures to game window size
				/*
				glBindTexture(GL_TEXTURE_2D, textureColorbuffer[0]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, game_scene_viewport_sizeX, game_scene_viewport_sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

				glBindTexture(GL_TEXTURE_2D, textureColorbuffer[1]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, game_scene_viewport_sizeX, game_scene_viewport_sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
				*/

				glBindTexture(GL_TEXTURE_2D, textures[0]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, game_scene_viewport_sizeX, game_scene_viewport_sizeY, 0, GL_RGBA, GL_FLOAT,
					NULL);
				glBindTexture(GL_TEXTURE_2D, textures[1]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, game_scene_viewport_sizeX, game_scene_viewport_sizeY, 0, GL_RGBA, GL_FLOAT,
					NULL);

				//set sample rate back to zero
				frame_count = 0.0;
			}

			ImGui::GetWindowDrawList()->AddImage(
				(void *)textures[1],
				ImVec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y),
				ImVec2(ImGui::GetCursorScreenPos().x + game_scene_viewport_sizeX,
					ImGui::GetCursorScreenPos().y + game_scene_viewport_sizeY), ImVec2(0, 1), ImVec2(1, 0));
			
			//ImGui::InvisibleButton("GameWindow", ImVec2(960, 580));
			

			//if hovering in the invisible button area
			if (ImGui::IsWindowHovered())
			{
				//track the mouse position only in the game window
				ImVec2 canvas_pos = ImGui::GetCursorScreenPos();

				ImVec2 mouse_pos_in_canvas = ImVec2(ImGui::GetIO().MousePos.x - canvas_pos.x, ImGui::GetIO().MousePos.y - canvas_pos.y);

				processInput(window);
			}

		}
		ImGui::End();

		ImGui::ShowDemoWindow();

		// Render dear imgui into screen
		user_interface.renderUI();


		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	//glDeleteVertexArrays(1, &cube.VAO);
	//glDeleteVertexArrays(1, &lightCube.VAO);
	//glDeleteBuffers(1, &cube.VBO);
	//glDeleteBuffers(1, &lightCube.VBO);

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);


	user_interface.shutdownUI();


	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		camera.ProcessKeyboard(DOWN, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		camera.ProcessKeyboard(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
		if (cursorCallbackMode == "locked") {
			//glfwSetWindowShouldClose(window, true); //closes window on escape
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			glfwSetCursorPosCallback(window, unlocked_mouse_callback);
			cursorCallbackMode = "unlocked";
		}
	}
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
		if (cursorCallbackMode == "unlocked") {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			glfwSetCursorPosCallback(window, mouse_callback);
			cursorCallbackMode = "locked";
			firstMouse = true;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true); //closes window on escape
	}

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void unlocked_mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	cout << xpos << ypos << endl;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		cout << "Cursor Position at (" << xpos << " : " << ypos << endl;
	}
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}
