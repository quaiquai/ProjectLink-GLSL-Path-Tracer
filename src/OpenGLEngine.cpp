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
#include "OBJ-Loader.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "path_tracing/pt_primitives.h"
#include "path_tracing/pt_chunk.h"

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
void processInput(GLFWwindow *window, float* frame_count);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void unlocked_mouse_callback(GLFWwindow* window, double xpos, double ypos);


// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;
string cursorCallbackMode = "unlocked";

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 0.0f));
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

float frame_count, time_count;
float angleX = 1.5;
float angleY = 0.0;

glm::vec3 cam = glm::vec3(0.0, 0.0, 0.0);

int lX = 0;
int lY = 0;

bool w_press = false;

glm::vec2 imouse = glm::vec2(0.0, 0.0);

std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;
std::vector< glm::vec3 > temp_vertices;
std::vector< glm::vec2 > temp_uvs;
std::vector< glm::vec3 > temp_normals;

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 1.0);
glm::vec3 cameraFwd = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 cameraMov = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraRight = glm::vec3(1.0f, 0.0f, 0.0f);

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
	glfwSetCursorPosCallback(window, unlocked_mouse_callback);
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
	Shader screenShader("shaders\\framebuffer_screen.vs", "shaders\\framebuffer_screen.fs");
	Shader pathtracingShader("shaders\\pathtracing_main.vs", "shaders\\pathtracing_main.fs");
	Shader pathtracingCompute("shaders\\compute\\pathtracing_compute.glsl");
	Shader finalCompute("shaders\\compute\\final.glsl");

	UI user_interface(window);

	pathtracingShader.use();

	// Initialize Loader
	objl::Loader Loader;

	// Load .obj File
	bool loadout = Loader.LoadFile("assets\\objects\\teapot.obj");

	objl::Mesh curMesh = Loader.LoadedMeshes[0];

	vector<float> vertices1 = { 10000.0, 10000.0, 10000.0, -10000.0, -10000.0, -10000.0 };
	vector<int> index;

	cout << Loader.LoadedMeshes.size();
	
	for (int k = 0; k < Loader.LoadedMeshes.size(); k++) {
		for (int i = 0; i < Loader.LoadedMeshes[k].Vertices.size(); i++) {
			vertices1.push_back(float(Loader.LoadedMeshes[k].Vertices[i].Position.X));
			vertices1.push_back(float(Loader.LoadedMeshes[k].Vertices[i].Position.Y));
			vertices1.push_back(float(Loader.LoadedMeshes[k].Vertices[i].Position.Z));

			(float(Loader.LoadedMeshes[k].Vertices[i].Position.X) < vertices1[0]) ? vertices1[0] = Loader.LoadedMeshes[k].Vertices[i].Position.X : vertices1[0] = vertices1[0];
			(float(Loader.LoadedMeshes[k].Vertices[i].Position.Y) < vertices1[1]) ? vertices1[1] = Loader.LoadedMeshes[k].Vertices[i].Position.Y : vertices1[1] = vertices1[1];
			(float(Loader.LoadedMeshes[k].Vertices[i].Position.Z) < vertices1[2]) ? vertices1[2] = Loader.LoadedMeshes[k].Vertices[i].Position.Z : vertices1[2] = vertices1[2];

			(float(Loader.LoadedMeshes[k].Vertices[i].Position.X) > vertices1[3]) ? vertices1[3] = Loader.LoadedMeshes[k].Vertices[i].Position.X : vertices1[3] = vertices1[3];
			(float(Loader.LoadedMeshes[k].Vertices[i].Position.Y) > vertices1[4]) ? vertices1[4] = Loader.LoadedMeshes[k].Vertices[i].Position.Y : vertices1[4] = vertices1[4];
			(float(Loader.LoadedMeshes[k].Vertices[i].Position.Z) > vertices1[5]) ? vertices1[5] = Loader.LoadedMeshes[k].Vertices[i].Position.Z : vertices1[5] = vertices1[5];
		}
	}
	cout << vertices1[0] << " " << vertices1[1] << " " << vertices1[2];

	for (int i = 0; i < curMesh.Indices.size(); i++) {

		index.push_back(float(curMesh.Indices[i]));
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

	pathtracingCompute.use();


	
	PT::Sphere sphere1(glm::vec3(12.0f, 5.0f, 20.0f), 1.0, glm::vec3(0.5, 0.5, 0.5), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.9, 0.9, 0.9), 0.4, 0.5);
	PT::Sphere sphere2(glm::vec3(-12.0f, 5.0f, 20.0f), 1.0, glm::vec3(0.5, 0.5, 0.5), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.9, 0.9, 0.9), 0.4, 0.5);

	std::vector<PT::Sphere> objects;

	objects.push_back(sphere1);
	objects.push_back(sphere2);

	std::vector<glm::vec3> sceneVerts;

	cout << vertices1.size() * sizeof(float);

	PT::Chunk world;
	float* vox = world.flatten();

	/*
	GLuint ssbo;
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, vertices1.size() * sizeof(float), &vertices1[0], GL_STATIC_READ); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
	*/
	
	


	//static variables used in imgui settings
	//translation position for the lamp cube representing the light
	static float translation[] = { 1.2f, 1.0f, 20.0f };
	//scale of the outer angle for the flashlight
	static float outerCutoff = 17.0;
	//the color of the diffuse component of the flashlight
	static float color[4] = { 0.7f,0.7f,0.7f,1.0f };

	unsigned int vox_world;
	glGenTextures(1, &vox_world);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_3D, vox_world);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, 16, 16, 16, 0, GL_RED, GL_FLOAT,
		vox);

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
	glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);


	unsigned int tex_output3;
	glGenTextures(1, &tex_output3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, tex_output3);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, game_scene_viewport_sizeX, game_scene_viewport_sizeY, 0, GL_RGBA, GL_FLOAT,
		NULL);
	glBindImageTexture(3, tex_output3, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);


	vector<string> textures_faces = {
		"assets\\textures\\aboveclouds\\px.png",
		"assets\\textures\\aboveclouds\\nx.png",
		"assets\\textures\\aboveclouds\\py.png",
		"assets\\textures\\aboveclouds\\ny.png",
		"assets\\textures\\aboveclouds\\pz.png",
		"assets\\textures\\aboveclouds\\nz.png"
	};


	//skybox
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	unsigned char *data;
	for (unsigned int i = 0; i < textures_faces.size(); i++)
	{
		data = stbi_load(textures_faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap tex failed to load at path: " << textures_faces[i] << std::endl;
			stbi_image_free(data);
		}
		
	}

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);




	frame_count = 0.0;
	time_count = 0.0;

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		

		// input
		// -----
		
		glfwPollEvents();

		// feed inputs to dear imgui, start new frame
		user_interface.newFrameUI();


		finalCompute.use();

		{ // launch compute shaders!
			finalCompute.setInt("game_window_x", int(game_scene_viewport_sizeX));
			finalCompute.setInt("game_window_y", int(game_scene_viewport_sizeY));
			glDispatchCompute(((GLuint)game_scene_viewport_sizeX + 8 - 1) / 8, ((GLuint)game_scene_viewport_sizeY + 8 - 1) / 8, 1);
		}

		// make sure writing to image has finished before read
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		pathtracingCompute.use();
		{ // launch compute shaders!
			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_3D, vox_world);
			pathtracingCompute.setVec2("iMouse", imouse);
			pathtracingCompute.setBool("w_press", w_press);
			pathtracingCompute.setVec2("camera", cam);
			pathtracingCompute.setVec3("sphereX", glm::vec3(translation[0], translation[1], translation[2]));
			pathtracingCompute.setVec3("cameraPos", glm::vec3(cameraPos[0], cameraPos[1], cameraPos[2]));
			pathtracingCompute.setVec3("cameraFwd", glm::vec3(cameraFwd[0], cameraFwd[1], cameraFwd[2]));
			pathtracingCompute.setVec3("cameraUp", glm::vec3(cameraUp[0], cameraUp[1], cameraUp[2]));
			pathtracingCompute.setVec3("cameraRight", glm::vec3(cameraRight[0], cameraRight[1], cameraRight[2]));
			pathtracingCompute.setVec3("cameraMov", glm::vec3(cameraMov[0], cameraMov[1], cameraMov[2]));
			pathtracingCompute.setFloat("game_window_x", game_scene_viewport_sizeX);
			pathtracingCompute.setFloat("game_window_y", game_scene_viewport_sizeY);
			pathtracingCompute.setFloat("iTime", time_count);
			pathtracingCompute.setFloat("iFrame", frame_count);
			pathtracingCompute.setFloat("angleX", angleX);
			pathtracingCompute.setFloat("angleY", angleY);
			glDispatchCompute(((GLuint)game_scene_viewport_sizeX + 8 - 1)/8, ((GLuint)game_scene_viewport_sizeY + 8 - 1) / 8, 1);
		}

		w_press = false;

		// make sure writing to image has finished before read
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		frame_count++;
		time_count++;


		ImGui::Begin("Triangle Position/Color");
		
		if (ImGui::SliderFloat3("sphereX", translation, -10.0, 50.0)) {
			frame_count = 0.0;
		}
		
		// color picker
		ImGui::ColorEdit3("color", color);
		ImGui::End();

		

		//user_interface.drawMenus();
		//for drawing ui to window
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

				glBindTexture(GL_TEXTURE_2D, tex_output);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, game_scene_viewport_sizeX, game_scene_viewport_sizeY, 0, GL_RGBA, GL_FLOAT,
					NULL);
				

				glBindTexture(GL_TEXTURE_2D, tex_output3);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, game_scene_viewport_sizeX, game_scene_viewport_sizeY, 0, GL_RGBA, GL_FLOAT,
					NULL);

				//set sample rate back to zero
				frame_count = 0.0;
			}

			ImGui::GetWindowDrawList()->AddImage(
				(void *)tex_output3,
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
				
				if(ImGui::GetMouseDragDelta().x != 0 || ImGui::GetMouseDragDelta().y != 0){
					//cout << float(ImGui::GetMouseDragDelta().x) * 0.1 << endl;
					frame_count = 0.0; 
					imouse.x = float(ImGui::GetMouseDragDelta().x - lX);
					imouse.y = float(lY - ImGui::GetMouseDragDelta().y);

				}
				if (lX != ImGui::GetMouseDragDelta().x || lY != ImGui::GetMouseDragDelta().y) {


					angleX = angleX - float(imouse.x * (3.14 / 180));
					angleY -= float(imouse.y * (3.14 / 180));



					if (angleY > 1.55334)
						angleY = 1.55334;
					if (angleY < -1.55334)
						angleY = -1.55334;

					cameraFwd.x = cos(angleX) * cos(angleY) * 1.0;
					cameraFwd.y = -sin(angleY) * 1.0;
					cameraFwd.z = sin(angleX) * cos(angleY) * 1.0;

					//cameraFwd += glm::vec3(0.0f, 0.0f, 0.0f);
					cameraPos = glm::normalize(cameraPos + cameraFwd);
					
					cameraRight = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), cameraPos));
					cameraUp = normalize(cross(cameraPos, cameraRight));
				}

				lX = ImGui::GetMouseDragDelta().x;
				lY = ImGui::GetMouseDragDelta().y;
				
				processInput(window, &frame_count);
				

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
void processInput(GLFWwindow *window, float* frame_count)
{
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		*frame_count = 0.0;
		w_press = true;
		cameraMov += cameraFwd * glm::vec3(0.1);
		camera.ProcessKeyboard(FORWARD, deltaTime);
		cout << "w Pressed";
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		*frame_count = 0.0;
		w_press = true;
		cameraMov -= cameraFwd * glm::vec3(0.1);
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	}
		
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		*frame_count = 0.0;
		cameraMov -= cameraRight * glm::vec3(0.1);
		camera.ProcessKeyboard(LEFT, deltaTime);
	}	
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		*frame_count = 0.0;
		cameraMov += cameraRight * glm::vec3(0.1);
		camera.ProcessKeyboard(RIGHT, deltaTime);
	}	
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		*frame_count = 0.0;
		cameraMov.y -= 0.1;
		camera.ProcessKeyboard(DOWN, deltaTime);
	}	
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		*frame_count = 0.0;
		cameraMov.y += 0.1;
		camera.ProcessKeyboard(UP, deltaTime);
	}
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
	frame_count = 0.0;
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	//cout << xoffset;
	

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void unlocked_mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	//cout << xpos << ypos << endl;
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
