#pragma once

#include "Window.hpp"

namespace engine {
	class FirstApp {
		Window window{ WIDTH, HEIGHT, "Hello, World!" };
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
