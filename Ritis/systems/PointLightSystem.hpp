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
#include <map>

#include "../Camera.hpp"

namespace engine {

	struct PointLightPushConstants {
		glm::vec4 position{};
		glm::vec4 color{};
		float radius;
	};

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

		auto update(FrameInfo&, GlobalUniformBufferObject&) -> void;
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
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // both vertex and fragment shader
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PointLightPushConstants);

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
	auto PointLightSystem::createPipeline(VkRenderPass renderPass) -> void {
		assert(this->pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
		// use swapchain width and height because they don't necessarily match the window
		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);
		Pipeline::enableAlphaBlending(pipelineConfig);
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
	auto PointLightSystem::update(FrameInfo& frameInfo, GlobalUniformBufferObject& ubo) -> void {
		auto rotateLight = glm::rotate(
			glm::mat4(1.0f),
			frameInfo.frameTime,
			{ 0.0f, -1.0f, 0.0f } // axis of rotation (up vector)
		);

		int lightIndex = 0;
		for (auto& [id, obj] : frameInfo.gameObjects) {
			if (obj.pointLight == nullptr) continue;

			assert(lightIndex < MAX_LIGHTS && "Point Lights Exceed maximum specified");

			// update light position
			obj.transform.translation = glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1.0f));
			
			// copy light to ubo
			ubo.pointLights[lightIndex].position = glm::vec4(obj.transform.translation, 1.0f);
			ubo.pointLights[lightIndex].color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
			lightIndex++;
		}
		ubo.numLights = lightIndex;
	}
	auto PointLightSystem::render(
		FrameInfo& frameInfo
	) -> void {
		std::map<float, GameObject::id_t> sorted;
		for (auto& [id, obj] : frameInfo.gameObjects) { // sort point lights by distance to always render back to front, allowing transparency
			if (obj.pointLight == nullptr) continue;	// if rendering front to back, then depth buffer (filled with front elements) will cause discarding of back elements
			// calculate distance						// making it look like nothing is behind and not really being transparent
			auto offset = frameInfo.camera.getPosition() - obj.transform.translation;
			float distSquared = glm::dot(offset, offset); // squared distance but close enough for sorting by distance purposes
			sorted[distSquared] = obj.getId();
		}
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

		// iterate through sorted lights in reverse order (back to front)
		for (auto it = sorted.rbegin(); it != sorted.rend(); it++) {
			// use game obj id to find light object
			auto& obj = frameInfo.gameObjects.at(it->second);

			PointLightPushConstants push{};
			push.position = glm::vec4(obj.transform.translation, 1.0f);
			push.color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
			push.radius = obj.transform.scale.x;

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				this->pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(PointLightPushConstants),
				&push
			);
			vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
		}
	}
}
