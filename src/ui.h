#ifndef UI_H
#define UI_H


#include "./shader.h"//has GLAD and should be before glfw
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


namespace UI{

	static bool view_attached_shaders = false;

	static void init(GLFWwindow* window) {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO &io = ImGui::GetIO();
		
		// Setup Platform/Renderer bindings
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 330");

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	}

	static void drawMenus(std::map<std::string, Shader> &list_of_shaders) {
		ImGui::BeginMainMenuBar();
		if (ImGui::BeginMenu("File"))
		{
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Shaders")) {
			ImGui::MenuItem("Show Attached Shaders", NULL, &view_attached_shaders);
			/*
			for (int i = 0; i < list_of_shaders.size(); ++i) {
				std::cout << i << std::endl;
			}
			*/
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	static void newFrameUI(std::map<std::string, Shader> &list_of_shaders) {
		// feed inputs to dear imgui, start new frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// render your GUI
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

		drawMenus(list_of_shaders);
	}

	static void renderUI() {
		// Render dear imgui into screen
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	static void drawShaderUI(Shader &shader) {
		for (auto &elements : shader.uniform_floats) {
			if (ImGui::SliderFloat(elements.first.c_str(), &elements.second, -10.0f, 50.0f)) {
				shader.uniform_floats["iFrame"] = 0.0f;
			};
		}

		for (auto &elements : shader.uniform_vec2) {
			if (ImGui::SliderFloat2(elements.first.c_str(), &elements.second[0], -10.0f, 50.0f)) {
				shader.uniform_floats["iFrame"] = 0.0f;
			};
		}

		for (auto &elements : shader.uniform_vec3) {
			if (ImGui::SliderFloat3(elements.first.c_str(), &elements.second[0], -10.0f, 50.0f)) {
				shader.uniform_floats["iFrame"] = 0.0f;
			};
		}
	}

	//we want to be able to pass in a shader and add it to a window of all attached shaders
	//that are being used
	//void drawattachedShaders(Shader &attachedShader) {

	//}

	static void shutdownUI() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	


};



#endif