#pragma once

#include "Window.hpp"
#include "GameObject.hpp"
#include "Renderer.hpp"
#include "SimpleRenderSystem.hpp"
#include "Camera.hpp"
#include "KeyboardMovementController.hpp"

#define GLM_FORCE_RADIANS					// functions expect radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// Depth buffer values will range from 0 to 1, not -1 to 1
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <memory>
#include <vector>
#include <stdexcept>
#include <array>
#include <chrono>

constexpr const float MAX_FRAME_TIME = 1.0f;

namespace engine {
	/*auto nextSierpinski(const std::vector<Model::Vertex>& prev) {
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
	}*/

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
		std::shared_ptr<Model> model = Model::createModelFromFile(device, "models/smooth_vase.obj");
		auto gameObj = GameObject::createGameObject();
		gameObj.model = model;
		gameObj.transform.translation = { 0.0f, 0.0f, 2.5f };
		gameObj.transform.scale = glm::vec3{3.0f};

		this->gameObjects.push_back(std::move(gameObj));
	}
	auto FirstApp::run() -> void {
		SimpleRenderSystem simpleRenderSystem{ this->device, this->renderer.getSwapChainRenderPass() };
		Camera camera{};
		camera.setViewTarget(glm::vec3(-1.0f, -2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 2.5f));

		auto viewerObject = GameObject::createGameObject(); // store camera state
		KeyboardMovementController cameraController{};

		auto currentTime = std::chrono::high_resolution_clock::now();

		while (!this->window.shouldClose()) {
			glfwPollEvents();

			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;

			frameTime = glm::min(frameTime, MAX_FRAME_TIME); // avoid really large skips if frames aren't coming in

			cameraController.moveInPlaneXZ(this->window.getGFLWWindow(), frameTime, viewerObject);
			camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

			float aspect = this->renderer.getAspectRatio();
			camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10);

			if (auto commandBuffer = this->renderer.beginFrame()) {
				this->renderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(commandBuffer, this->gameObjects, camera);
				this->renderer.endSwapChainRenderPass(commandBuffer);
				this->renderer.endFrame();
			}
		}
		vkDeviceWaitIdle(this->device.device());
	}
}
