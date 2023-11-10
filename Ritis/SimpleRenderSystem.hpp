#pragma once

#include "Device.hpp"
#include "Pipeline.hpp"
#include "GameObject.hpp"

#define GLM_FORCE_RADIANS					// functions expect radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// Depth buffer values will range from 0 to 1, not -1 to 1
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <memory>
#include <vector>
#include <stdexcept>
#include <array>

#include "Camera.hpp"

namespace engine {

	// push constants have a particular memory layout,
	// vec3 must be aligned to a multiple of 16 by vulkan docs
	// vec2 takes up 8, so pad 8 more then place vec 3
	struct SimplePushConstantData {
		glm::mat4 transform{1.0f};
		alignas(16) glm::vec3 color;
	};

	class SimpleRenderSystem {
		Device& device;

		std::unique_ptr<Pipeline> pipeline;
		VkPipelineLayout pipelineLayout;

		auto createPipelineLayout() -> void;
		auto createPipeline(VkRenderPass) -> void;
	public:
		SimpleRenderSystem(Device&, VkRenderPass);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		auto renderGameObjects(VkCommandBuffer, std::vector<GameObject>&, const Camera&) -> void;
		auto run() -> void;
	};

	SimpleRenderSystem::SimpleRenderSystem(Device& d, VkRenderPass renderPass) : device{ d } {
		this->createPipelineLayout();
		this->createPipeline(renderPass);
	}
	SimpleRenderSystem::~SimpleRenderSystem() {
		vkDestroyPipelineLayout(this->device.device(), this->pipelineLayout, nullptr);
	}

	auto SimpleRenderSystem::createPipelineLayout() -> void {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // both vertex and fragment shader
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		pipelineLayoutInfo.setLayoutCount = 0;						// empty layout, layouts can be used to send additional data
		pipelineLayoutInfo.pSetLayouts = nullptr;					// to vertex and fragment shaders (textures, uniform buffer objs, .etc)

		pipelineLayoutInfo.pushConstantRangeCount = 1;				// method of sending small amounts of data to shader programs
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (
			vkCreatePipelineLayout(
				this->device.device(),
				&pipelineLayoutInfo,
				nullptr,				// allocation callback
				&this->pipelineLayout
			) != VK_SUCCESS
			) {
			throw std::runtime_error("Failed to create pipeline layout");
		}
	}
	auto SimpleRenderSystem::createPipeline(VkRenderPass renderPass) -> void {
		assert(this->pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
		// use swapchain width and height because they don't necessarily match the window
		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass; // render pass describes structure and format of frame buffer objects
		pipelineConfig.pipelineLayout = this->pipelineLayout;
		this->pipeline = std::make_unique<Pipeline>(
			device,
			"shaders/simpleShader.vert.spv",
			"shaders/simpleShader.frag.spv",
			pipelineConfig
		);
	}
	auto SimpleRenderSystem::renderGameObjects(
		VkCommandBuffer commandBuffer,
		std::vector<GameObject>& gameObjects,
		const Camera& camera)
	-> void {
		this->pipeline->bind(commandBuffer);
		auto projectionView = camera.getProjection() * camera.getView();
		for (auto& obj : gameObjects) {
			obj.transform.rotation.y = glm::mod(obj.transform.rotation.y + 0.0001f, glm::two_pi<float>());
			obj.transform.rotation.x = glm::mod(obj.transform.rotation.x + 0.00005f, glm::two_pi<float>());

			SimplePushConstantData push{};
			push.color = obj.color;
			push.transform = projectionView * obj.transform.mat4();

			vkCmdPushConstants(
				commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push
			);
			obj.model->bind(commandBuffer);
			obj.model->draw(commandBuffer);
		}
	}
}

