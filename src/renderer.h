#pragma once
#include <glad/glad.h>


namespace lRenderer {

	inline void enbaleDepthTesting() {
		// configure global opengl state
		// -----------------------------
		glEnable(GL_DEPTH_TEST);
	}

	inline void disableDepthTesting() {
		// configure global opengl state
		// -----------------------------
		glDisable(GL_DEPTH_TEST);
	}

	inline int renderLoop(Window &window) {

		window.pollEvents();

		while (!window.checkWindowClose()) {

		}
	}

}