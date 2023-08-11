#pragma once

#include "Window.hpp"
#include "Pipeline.hpp"

namespace engine {
	class FirstApp {
		Window window{ WIDTH, HEIGHT, "Hello, World!" };
		Pipeline pipeline{ "shaders/simpleShader.vert.spv", "shaders/simpleShader.frag.spv" };
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		auto run() -> void;
	};

	auto FirstApp::run() -> void {
		while (!window.shouldClose()) {
			glfwPollEvents();
		}
	}
}
