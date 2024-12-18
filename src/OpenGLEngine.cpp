// OpenGLEngine.cpp : Defines the entry point for the console application.
//

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "path_tracing/pt_camera.h"

#include <algorithm>
#include <iterator>
#include "OBJ-Loader.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "path_tracing/pt_primitives.h"
#include "path_tracing/pt_chunk.h"

#include "window.h"
#include "renderer.h"
#include "shader.h"
#include "camera.h"
#include "primitives.h"
#include "ui.h"
#include "texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "globals.h"
using namespace std;

// camera
//Camera camera(glm::vec3(0.0f, 0.0f, 0.0f));
//PTCamera camera(50.f, glm::vec3(0.0f, 0.0f, 1.0f));

//lighting Position
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

unsigned int VBO;
unsigned int VAO;

float sphereX = 0.0;

float time_count;
float angleX = 1.5;
float angleY = 0.0;

glm::vec3 cam = glm::vec3(0.0, 0.0, 0.0);

int lX = 0;
int lY = 0;

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

	Window window(gLink::SCR_WIDTH, gLink::SCR_HEIGHT, "LearnOpenGL");

	gLink::initGLAD	();
	lRenderer::enbaleDepthTesting();
	
	// build and compile our shader zprogram
	// ------------------------------------
	Shader screenShader("Screen Shader", "shaders\\framebuffer_screen.vs", "shaders\\framebuffer_screen.fs");
	Shader pathtracingShader("Path tracing shader", "shaders\\pathtracing_main.vs", "shaders\\pathtracing_main.fs");
	Shader pathtracingCompute("Path tracing compute shader", "shaders\\compute\\pathtracing_compute.glsl");
	Shader finalCompute("Post proccessing", "shaders\\compute\\final.glsl");

	UI::init(window.window);

	pathtracingShader.use();

	// Initialize Loader
	objl::Loader Loader;

	// Load .obj File
	bool loadout = Loader.LoadFile("assets\\objects\\teapot.obj");

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
	Texture inputTexture;
	inputTexture.bind(0);
	inputTexture.createTexture(gLink::game_scene_viewport_sizeX, gLink::game_scene_viewport_sizeY, GL_RGBA32F, GL_RGBA, nullptr);
	inputTexture.unbind();

	Texture outputTexture;
	outputTexture.bind(3);
	outputTexture.createTexture(gLink::game_scene_viewport_sizeX, gLink::game_scene_viewport_sizeY, GL_RGBA32F, GL_RGBA, nullptr);
	outputTexture.unbind();

	


	vector<string> textures_faces = {
		"assets\\textures\\aboveclouds\\px.png",
		"assets\\textures\\aboveclouds\\nx.png",
		"assets\\textures\\aboveclouds\\py.png",
		"assets\\textures\\aboveclouds\\ny.png",
		"assets\\textures\\aboveclouds\\pz.png",
		"assets\\textures\\aboveclouds\\nz.png"
	};


	//skybox
	stbi_set_flip_vertically_on_load(true);
	int width, height, nrComponents;
	float *data = stbi_loadf("assets\\textures\\cubemaps\\partly_cloudy.hdr", &width, &height, &nrComponents, 0);
	Texture hdrTexture;
	float highest = -100.0;
	float brightestUV[2] = { 0,0 };
	if (data)
	{

		
		hdrTexture.bind(7);
		hdrTexture.createTexture(width, height, GL_RGB16F, GL_RGB, data);
		hdrTexture.unbind();

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Failed to load HDR image." << std::endl;
	}


	/*
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
	*/


	pathtracingCompute.setInt("test_int", 10);

	gLink::frame_count = 0.0;
	time_count = 0.0;

	pathtracingCompute.setVec2("iMouse", imouse);
	pathtracingCompute.setBool("w_press", gLink::w_press);
	pathtracingCompute.setVec3("sphereX", glm::vec3(translation[0], translation[1], translation[2]));
	pathtracingCompute.setFloat("game_window_x", gLink::game_scene_viewport_sizeX);
	pathtracingCompute.setFloat("game_window_y", gLink::game_scene_viewport_sizeY);
	pathtracingCompute.setFloat("angleX", angleX);
	pathtracingCompute.setFloat("angleY", angleY);

	pathtracingCompute.attachCamera(gLink::camera);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window.window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		gLink::deltaTime = currentFrame - gLink::lastFrame;
		gLink::lastFrame = currentFrame;
		

		// input
		// -----
		
		glfwPollEvents();

		// feed inputs to dear imgui, start new frame
		UI::newFrameUI(Shader::createdShaders);


		finalCompute.use();

		

		{ // launch compute shaders!
			finalCompute.setInt("game_window_x", int(gLink::game_scene_viewport_sizeX));
			finalCompute.setInt("game_window_y", int(gLink::game_scene_viewport_sizeY));
			finalCompute.updateUniforms();
			glDispatchCompute(((GLuint)gLink::game_scene_viewport_sizeX + 8 - 1) / 8, ((GLuint)gLink::game_scene_viewport_sizeY + 8 - 1) / 8, 1);
		}

		// make sure writing to image has finished before read
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		pathtracingCompute.use();
		
		
		{ // launch compute shaders!
			//OG skybox
			//glActiveTexture(GL_TEXTURE6);
			//glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
			glActiveTexture(GL_TEXTURE7);
			glBindTexture(GL_TEXTURE_2D, hdrTexture.id);
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_3D, vox_world);
			
			glDispatchCompute(((GLuint)gLink::game_scene_viewport_sizeX + 8 - 1)/8, ((GLuint)gLink::game_scene_viewport_sizeY + 8 - 1) / 8, 1);
		}

		gLink::w_press = false;

		// make sure writing to image has finished before read
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		gLink::frame_count++;
		time_count++;


		ImGui::Begin("Triangle Position/Color");
		
		if (ImGui::SliderFloat3("sphereX", translation, -10.0, 50.0)) {
			gLink::frame_count = 0.0;
		}
		
		// color picker
		ImGui::ColorEdit3("color", color);
		ImGui::End();

		

		//user_interface.drawMenus();
		//for drawing ui to window
		glViewport(0, 0, gLink::SCR_WIDTH, gLink::SCR_HEIGHT);
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

			if (window_size.x != gLink::game_scene_viewport_sizeX && window_size.y != gLink::game_scene_viewport_sizeY) {
				//resize has happened or first inital render
				gLink::game_scene_viewport_sizeX = window_size.x;
				gLink::game_scene_viewport_sizeY = window_size.y;

				glBindTexture(GL_TEXTURE_2D, inputTexture.id);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, gLink::game_scene_viewport_sizeX, gLink::game_scene_viewport_sizeY, 0, GL_RGBA, GL_FLOAT,
					NULL);
				

				glBindTexture(GL_TEXTURE_2D, outputTexture.id);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, gLink::game_scene_viewport_sizeX, gLink::game_scene_viewport_sizeY, 0, GL_RGBA, GL_FLOAT,
					NULL);

				//set sample rate back to zero
				gLink::frame_count = 0.0;
			}

			ImGui::GetWindowDrawList()->AddImage(
				(void *)outputTexture.id,
				ImVec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y),
				ImVec2(ImGui::GetCursorScreenPos().x + gLink::game_scene_viewport_sizeX,
					ImGui::GetCursorScreenPos().y + gLink::game_scene_viewport_sizeY), ImVec2(0, 1), ImVec2(1, 0));
			
			//ImGui::InvisibleButton("GameWindow", ImVec2(960, 580));
			

			//if hovering in the invisible button area
			if (ImGui::IsWindowHovered())
			{
				//track the mouse position only in the game window
				ImVec2 canvas_pos = ImGui::GetCursorScreenPos();

				ImVec2 mouse_pos_in_canvas = ImVec2(ImGui::GetIO().MousePos.x - canvas_pos.x, ImGui::GetIO().MousePos.y - canvas_pos.y);
				
				if(ImGui::GetMouseDragDelta().x != 0 || ImGui::GetMouseDragDelta().y != 0){
					//cout << float(ImGui::GetMouseDragDelta().x) * 0.1 << endl;
					gLink::frame_count = 0.0; 
					imouse.x = float(ImGui::GetMouseDragDelta().x - lX);
					imouse.y = float(lY - ImGui::GetMouseDragDelta().y);
					pathtracingCompute.uniform_vec2["iMouse"] = imouse;
				}
				if (lX != ImGui::GetMouseDragDelta().x || lY != ImGui::GetMouseDragDelta().y) {


					angleX = angleX - float(imouse.x * (3.14 / 180));
					angleY -= float(imouse.y * (3.14 / 180));



					if (angleY > 1.55334)
						angleY = 1.55334;
					if (angleY < -1.55334)
						angleY = -1.55334;

					gLink::camera.cameraFwd.x = cos(angleX) * cos(angleY) * 1.0;
					gLink::camera.cameraFwd.y = -sin(angleY) * 1.0;
					gLink::camera.cameraFwd.z = sin(angleX) * cos(angleY) * 1.0;

					//cameraFwd += glm::vec3(0.0f, 0.0f, 0.0f);
					gLink::camera.cameraPos = glm::normalize(gLink::camera.cameraPos + gLink::camera.cameraFwd);
					
					gLink::camera.cameraRight = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), gLink::camera.cameraPos));
					gLink::camera.cameraUp = normalize(cross(gLink::camera.cameraPos, gLink::camera.cameraRight));
					pathtracingCompute.uniform_floats["iFrame"] = 0.f;
					pathtracingCompute.attachCamera(gLink::camera);
				}

				lX = ImGui::GetMouseDragDelta().x;
				lY = ImGui::GetMouseDragDelta().y;
				
				Window::processInput(window.window, &gLink::frame_count);

				if (gLink::w_press) { pathtracingCompute.uniform_floats["iFrame"] = 0.f; }
				gLink::w_press - false;
				pathtracingCompute.setFloat("game_window_x", gLink::game_scene_viewport_sizeX);
				pathtracingCompute.setFloat("game_window_y", gLink::game_scene_viewport_sizeY);
				pathtracingCompute.attachCamera(gLink::camera);
			}

		}
		ImGui::End();

		ImGui::ShowDemoWindow();

		UI::drawShaderUI(pathtracingCompute);
		pathtracingCompute.updateUniforms();

		// Render dear imgui into screen
		UI::renderUI();
		std::cout << UI::view_attached_shaders << std::endl;

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		
		glfwSwapBuffers(window.window);
		
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);


	UI::shutdownUI();


	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

