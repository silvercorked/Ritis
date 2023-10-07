#pragma once

#include "Window.hpp"
#include "GameObject.hpp"
#include "Renderer.hpp"
#include "SimpleRenderSystem.hpp"

#define GLM_FORCE_RADIANS					// functions expect radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// Depth buffer values will range from 0 to 1, not -1 to 1
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <memory>
#include <vector>
#include <stdexcept>
#include <array>

namespace engine {

	auto nextSierpinski(const std::vector<Model::Vertex>& prev) {
		std::vector<Model::Vertex> next = {};
		for (auto i = 0; i < prev.size(); i += 3) { // prev[i][i + 1] and [i + 2] hold the triangl's verticies
			glm::vec2 avg0_1 = (prev[i].position + prev[i + 1].position) / 2.0f; // mid-point between vertex 0 and 1
			glm::vec2 avg0_2 = (prev[i].position + prev[i + 2].position) / 2.0f; // mid-point between vertex 0 and 2
			glm::vec2 avg1_2 = (prev[i + 1].position + prev[i + 2].position) / 2.0f; // mid-point between vertex 1 and 2
			{	// first triangle (0th vertex + 01 midpoint + 02 midpoint)
				next.push_back({ prev[i].position , prev[i].color });
				next.push_back({ avg0_1, prev[i + 1].color });
				next.push_back({ avg0_2, prev[i + 2].color });
			}
			{	// second (1st vertex + 01 midpoint + 12 midpoint)
				next.push_back({ prev[i + 1].position , prev[i + 1].color });
				next.push_back({ avg0_1, prev[i].color });
				next.push_back({ avg1_2, prev[i + 2].color });
			}
			{	// third (2nd vertex + 02 midpoint + 12 midpoint)
				next.push_back({ prev[i + 2].position , prev[i + 2].color });
				next.push_back({ avg0_2, prev[i].color });
				next.push_back({ avg1_2, prev[i + 1].color });
			}
		}
		return next;
	}

	class FirstApp {
		Window window{ WIDTH, HEIGHT, "Hello, World!" };
		Device device{ window };
		Renderer renderer{ window, device };

		std::vector<GameObject> gameObjects;

		auto loadGameObjects() -> void;
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;
		
		FirstApp();
		~FirstApp();

		FirstApp(const FirstApp&) = delete;
		FirstApp& operator=(const FirstApp&) = delete;

		auto run() -> void;
	};

	FirstApp::FirstApp() {
		this->loadGameObjects();
	}
	FirstApp::~FirstApp() {}

	auto FirstApp::loadGameObjects() -> void {
		const glm::vec2 top = { -0.98f, -0.01f };
		const glm::vec2 right = { -0.97f, 0.0f };
		const glm::vec2 left = { -0.99f, 0.0f };
		std::vector<Model::Vertex> verticies {
			{ {-0.98f, -0.5f}, { 1.0f, 0.0f, 0.0f } },
			{ {0.5f, -0.4f}, {0.0f, 1.0f, 0.0f} },
			{ {0.3f, 0.4f}, {0.0f, 0.0f, 1.0f} }
		};
		//const auto SeirpinskiDepth = 2;
		//for (auto i = 0; i < SeirpinskiDepth; i++)
		//	verticies = nextSierpinski(verticies);

		auto model = std::make_shared<Model>(this->device, verticies);

		auto triangle = GameObject::createGameObject();
		triangle.model = model;
		triangle.color = { 0.1f, 0.8f, 0.1f };
		triangle.transform2d.translation.x = 0.0f;
		triangle.transform2d.scale = { 1.0f, 1.0f };
		triangle.transform2d.rotation = 0.0f * glm::two_pi<float>();

		this->gameObjects.push_back(std::move(triangle));
	}
	auto FirstApp::run() -> void {
		SimpleRenderSystem simpleRenderSystem{ this->device, this->renderer.getSwapChainRenderPass() };
		while (!this->window.shouldClose()) {
			glfwPollEvents();
			if (auto commandBuffer = this->renderer.beginFrame()) {
				this->renderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(commandBuffer, this->gameObjects);
				this->renderer.endSwapChainRenderPass(commandBuffer);
				this->renderer.endFrame();
			}
		}

		vkDeviceWaitIdle(this->device.device());
	}
}
