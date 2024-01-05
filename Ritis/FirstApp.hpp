#pragma once

#include "Window.hpp"
#include "GameObject.hpp"
#include "Renderer.hpp"
#include "SimpleRenderSystem.hpp"
#include "Buffer.hpp"
#include "Camera.hpp"
#include "KeyboardMovementController.hpp"
#include "Descriptors.hpp"

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

	struct GlobalUniformBufferObject {
		glm::mat4 projectionView{ 1.0f };
		glm::vec3 lightDirection = glm::normalize(glm::vec3{ 1.0f, -3.0f, -1.0f });
	};

	class FirstApp {
		Window window{ WIDTH, HEIGHT, "Vulkan Learning" };
		Device device{ window };
		Renderer renderer{ window, device };

		// order of declarations matters
		std::unique_ptr<DescriptorPool> globalPool{}; // pool needs to be destroyed before devices
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
		this->globalPool = DescriptorPool::Builder(this->device)
			.setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
			.build();
		this->loadGameObjects();
	}
	FirstApp::~FirstApp() {}

	auto FirstApp::loadGameObjects() -> void {
		std::shared_ptr<Model> flatModel = Model::createModelFromFile(device, "models/flat_vase.obj");
		auto gameObj1 = GameObject::createGameObject();
		gameObj1.model = flatModel;
		gameObj1.transform.translation = { -0.5f, 0.5f, 2.5f };
		gameObj1.transform.scale = glm::vec3{ 3.0f, 1.5f, 3.0f };

		this->gameObjects.push_back(std::move(gameObj1));

		std::shared_ptr<Model> smoothModel = Model::createModelFromFile(device, "models/smooth_vase.obj");
		auto gameObj2 = GameObject::createGameObject();
		gameObj2.model = smoothModel;
		gameObj2.transform.translation = { 0.5f, 0.5f, 2.5f };
		gameObj2.transform.scale = glm::vec3{ 3.0f, 1.5f, 3.0f };

		this->gameObjects.push_back(std::move(gameObj2));
	}
	auto FirstApp::run() -> void {
		std::vector<std::unique_ptr<Buffer>> uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < uboBuffers.size(); i++) {
			uboBuffers[i] = std::make_unique<Buffer>(
				this->device,
				sizeof(GlobalUniformBufferObject),
				1, // write to frame's instance without dealing with sync issues (say frame 1 is rendering, can still store UBO for frame 2)
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT // no coherent bit, opt to use flush instead, even though 1 buffer per swapchain frame
			);
			uboBuffers[i]->map();
		}

		auto globalSetLayout = DescriptorSetLayout::Builder(this->device)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.build();

		std::vector<VkDescriptorSet> globalDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			DescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.build(globalDescriptorSets[i]);
		}

		SimpleRenderSystem simpleRenderSystem{ this->device, this->renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
		Camera camera{};
		camera.setViewTarget(glm::vec3(-1.0f, -2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 2.5f));

		auto viewerObject = GameObject::createGameObject(); // store camera state
		KeyboardMovementController cameraController{};

		auto currentTime = std::chrono::high_resolution_clock::now();

		uint32_t frameCount = 0;
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
				int frameIndex = renderer.getFrameIndex();
				FrameInfo frameInfo{
					frameIndex,
					frameTime,
					commandBuffer,
					camera,
					globalDescriptorSets[frameIndex]
				};
				// update
				GlobalUniformBufferObject ubo{};
				//float rotation = glm::cos((static_cast<float>(frameCount++) / 1000.0f));
				//ubo.lightDirection = glm::normalize(glm::vec3{ 1.0f * rotation, -1.0f, -1.0f * rotation });
				ubo.projectionView = camera.getProjection() * camera.getView();
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				// render
				this->renderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(frameInfo, this->gameObjects);
				this->renderer.endSwapChainRenderPass(commandBuffer);
				this->renderer.endFrame();
			}
		}
		vkDeviceWaitIdle(this->device.device());
	}
}
