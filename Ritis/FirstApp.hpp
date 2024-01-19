#pragma once

#include "Window.hpp"
#include "GameObject.hpp"
#include "Renderer.hpp"
#include "systems/SimpleRenderSystem.hpp"
#include "systems/PointLightSystem.hpp"
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
	class FirstApp {
		Window window{ WIDTH, HEIGHT, "Vulkan Learning" };
		Device device{ window };
		Renderer renderer{ window, device };

		// order of declarations matters
		std::unique_ptr<DescriptorPool> globalPool{}; // pool needs to be destroyed before devices
		GameObject::Map gameObjects;

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
		gameObj1.transform.translation = { -0.5f, 0.5f, 0.0f };
		gameObj1.transform.scale = glm::vec3{ 3.0f, 1.5f, 3.0f };

		this->gameObjects.emplace(gameObj1.getId(), std::move(gameObj1));

		std::shared_ptr<Model> smoothModel = Model::createModelFromFile(device, "models/smooth_vase.obj");
		auto gameObj2 = GameObject::createGameObject();
		gameObj2.model = smoothModel;
		gameObj2.transform.translation = { 0.5f, 0.5f, 0.0f };
		gameObj2.transform.scale = glm::vec3{ 3.0f, 1.5f, 3.0f };

		this->gameObjects.emplace(gameObj2.getId(), std::move(gameObj2));

		std::shared_ptr<Model> floorModel = Model::createModelFromFile(device, "models/quad.obj");
		auto floor = GameObject::createGameObject();
		floor.model = floorModel;
		floor.transform.translation = { 0.0f, 0.5f, 0.0f };
		floor.transform.scale = { 3.0f, 1.0f, 3.0f };

		this->gameObjects.emplace(floor.getId(), std::move(floor));

		//auto pointLight = GameObject::makePointLight(0.2f);
		//this->gameObjects.emplace(pointLight.getId(), std::move(pointLight));
		std::vector<glm::vec3> lightColors{
			{1.0f, 0.1f, 0.1f},
			{0.1f, 0.1f, 1.0f},
			{0.1f, 1.0f, 0.1f},
			{1.0f, 1.0f, 0.1f},
			{0.1f, 1.0f, 1.0f},
			{1.0f, 1.0f, 1.0f}
		};
		for (int i = 0; i < lightColors.size(); i++) {
			auto pointLight = GameObject::makePointLight(0.2f);
			pointLight.color = lightColors[i];
			auto rotateLight = glm::rotate(
				glm::mat4(1.0f),
				(i * glm::two_pi<float>()) / lightColors.size(),
				{0.0f, -1.0f, 0.0f} // axis of rotation (up vector)
			);
			pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f));
			gameObjects.emplace(pointLight.getId(), std::move(pointLight));
		}
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
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.build(); // VK_SHADER_STAGE_ALL_GRAPHICS = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT

		std::vector<VkDescriptorSet> globalDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			DescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.build(globalDescriptorSets[i]);
		}

		SimpleRenderSystem simpleRenderSystem{
			this->device,
			this->renderer.getSwapChainRenderPass(),
			globalSetLayout->getDescriptorSetLayout()
		};
		PointLightSystem pointLightSystem{
			this->device,
			this->renderer.getSwapChainRenderPass(),
			globalSetLayout->getDescriptorSetLayout()
		};

		Camera camera{};
		camera.setViewTarget(glm::vec3(-1.0f, -2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 2.5f));

		auto viewerObject = GameObject::createGameObject(); // store camera state
		viewerObject.transform.translation.z = -2.5f;
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
					globalDescriptorSets[frameIndex],
					gameObjects
				};
				// update
				GlobalUniformBufferObject ubo{};
				ubo.projection = camera.getProjection();
				ubo.view = camera.getView();
				ubo.inverseView = camera.getInverseView();
				pointLightSystem.update(frameInfo, ubo);
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				// render
				this->renderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(frameInfo);
				pointLightSystem.render(frameInfo);
				this->renderer.endSwapChainRenderPass(commandBuffer);
				this->renderer.endFrame();
			}
		}
		vkDeviceWaitIdle(this->device.device());
	}
}
