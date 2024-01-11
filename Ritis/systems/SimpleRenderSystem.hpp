#pragma once

#include "../Device.hpp"
#include "../Pipeline.hpp"
#include "../GameObject.hpp"
#include "../FrameInfo.hpp"

#define GLM_FORCE_RADIANS					// functions expect radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// Depth buffer values will range from 0 to 1, not -1 to 1
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <memory>
#include <vector>
#include <stdexcept>
#include <array>

#include "../Camera.hpp"

namespace engine {

	// push constants have a particular memory layout,
	// vec3 must be aligned to a multiple of 16 by vulkan docs
	// vec2 takes up 8, so pad 8 more then place vec 3
	struct SimplePushConstantData {
		glm::mat4 modelMatrix{1.0f};
		glm::mat4 normalMatrix{1.0f}; // still mat4 for alignment
	};

	class SimpleRenderSystem {
		Device& device;

		std::unique_ptr<Pipeline> pipeline;
		VkPipelineLayout pipelineLayout;

		auto createPipelineLayout(VkDescriptorSetLayout) -> void;
		auto createPipeline(VkRenderPass) -> void;
	public:
		SimpleRenderSystem(Device&, VkRenderPass, VkDescriptorSetLayout);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		auto renderGameObjects(FrameInfo&) -> void;
		auto run() -> void;
	};

	SimpleRenderSystem::SimpleRenderSystem(Device& d, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : device{ d } {
		this->createPipelineLayout(globalSetLayout);
		this->createPipeline(renderPass);
	}
	SimpleRenderSystem::~SimpleRenderSystem() {
		vkDestroyPipelineLayout(this->device.device(), this->pipelineLayout, nullptr);
	}

	auto SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) -> void {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // both vertex and fragment shader
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

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
		FrameInfo& frameInfo
	) -> void {
		this->pipeline->bind(frameInfo.commandBuffer);

		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			this->pipelineLayout,
			0, 1,					// which descriptor set to bind and how many to bind (bind 0th, and bind only 1). all bound after 0th are undone, so want earliest ones to be the ones that need to rebind least commonly
			&frameInfo.globalDescriptorSet,
			0,
			nullptr
		);

		for (auto& [id, obj] : frameInfo.gameObjects) {
			if (obj.model == nullptr) continue;
			SimplePushConstantData push{};
			push.modelMatrix = obj.transform.mat4();
			push.normalMatrix = obj.transform.normalMatrix(); // auto convert mat3 -> padded mat4

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push
			);
			obj.model->bind(frameInfo.commandBuffer);
			obj.model->draw(frameInfo.commandBuffer);
		}
	}
}

