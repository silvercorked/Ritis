#pragma once

#include "Window.hpp"
#include "Pipeline.hpp"
#include "Device.hpp"

namespace engine {
	class FirstApp {
		Window window{ WIDTH, HEIGHT, "Hello, World!" };
		Device device{ window };
		Pipeline pipeline{
			device,
			"shaders/simpleShader.vert.spv",
			"shaders/simpleShader.frag.spv",
			Pipeline::defaultPipelineConfigInfo(WIDTH, HEIGHT)
		};
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
