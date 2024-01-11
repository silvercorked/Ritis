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

	class PointLightSystem {
		Device& device;

		std::unique_ptr<Pipeline> pipeline;
		VkPipelineLayout pipelineLayout;

		auto createPipelineLayout(VkDescriptorSetLayout) -> void;
		auto createPipeline(VkRenderPass) -> void;
	public:
		PointLightSystem(Device&, VkRenderPass, VkDescriptorSetLayout);
		~PointLightSystem();

		PointLightSystem(const PointLightSystem&) = delete;
		PointLightSystem& operator=(const PointLightSystem&) = delete;

		auto render(FrameInfo&) -> void;
		auto run() -> void;
	};

	PointLightSystem::PointLightSystem(Device& d, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : device{ d } {
		this->createPipelineLayout(globalSetLayout);
		this->createPipeline(renderPass);
	}
	PointLightSystem::~PointLightSystem() {
		vkDestroyPipelineLayout(this->device.device(), this->pipelineLayout, nullptr);
	}

	auto PointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) -> void {
		//VkPushConstantRange pushConstantRange{};
		//pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // both vertex and fragment shader
		//pushConstantRange.offset = 0;
		//pushConstantRange.size = sizeof(SimplePushConstantData);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

		pipelineLayoutInfo.pushConstantRangeCount = 0;				// method of sending small amounts of data to shader programs
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

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
	auto PointLightSystem::createPipeline(VkRenderPass renderPass) -> void {
		assert(this->pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
		// use swapchain width and height because they don't necessarily match the window
		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.attributeDescriptions.clear();
		pipelineConfig.bindingDescriptions.clear();
		pipelineConfig.renderPass = renderPass; // render pass describes structure and format of frame buffer objects
		pipelineConfig.pipelineLayout = this->pipelineLayout;
		this->pipeline = std::make_unique<Pipeline>(
			device,
			"shaders/pointLight.vert.spv",
			"shaders/pointLight.frag.spv",
			pipelineConfig
		);
	}
	auto PointLightSystem::render(
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

		vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
	}
}

