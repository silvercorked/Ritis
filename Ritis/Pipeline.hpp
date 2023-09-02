#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cassert>

#include "Device.hpp"
#include "Model.hpp"

namespace engine {
	struct PipelineConfigInfo {
		VkViewport									viewport;							// describes how we want to transform our gl_position values to output image
		VkRect2D									scissor;
		VkPipelineInputAssemblyStateCreateInfo		inputAssemblyInfo;
		VkPipelineRasterizationStateCreateInfo		rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo		multisampleInfo;
		VkPipelineColorBlendAttachmentState			colorBlendAttachement;
		VkPipelineColorBlendStateCreateInfo			colorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo		depthStencilInfo;
		VkPipelineLayout							pipelineLayout			= nullptr;
		VkRenderPass								renderPass				= nullptr;
		uint32_t									subpass					= 0;
	};
	class Pipeline {
		Device& device;
		VkPipeline graphicsPipeline;
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;

		static auto readFile(const std::string& filepath) -> std::vector<char>;
		auto createGraphicsPipeline(const std::string& vertFilepath, const std::string& fragFilepath, const PipelineConfigInfo& config) -> void;
		auto createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule) -> void;
	public:
		Pipeline(Device& device, const std::string& vertFilepath, const std::string& fragFilepath, const PipelineConfigInfo& config);
		~Pipeline();
		Pipeline(const Pipeline&) = delete;
		auto operator=(const Pipeline&) = delete;

		auto bind(VkCommandBuffer commandBuffer) -> void;
		static auto defaultPipelineConfigInfo(uint32_t width, uint32_t height) -> PipelineConfigInfo;
	};

	Pipeline::Pipeline(
		Device& device,
		const std::string& vertFilepath,
		const std::string& fragFilepath,
		const PipelineConfigInfo& config
	) :
		device{device}
	{
		createGraphicsPipeline(vertFilepath, fragFilepath, config);
	}
	Pipeline::~Pipeline() {
		vkDestroyShaderModule(this->device.device(), vertShaderModule, nullptr);
		vkDestroyShaderModule(this->device.device(), fragShaderModule, nullptr);
		vkDestroyPipeline(this->device.device(), graphicsPipeline, nullptr);
	}

	auto Pipeline::readFile(const std::string& filepath) -> std::vector<char> {
		std::ifstream file{filepath, std::ios::ate | std::ios::binary}; // jump to end and as binary
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file: " + filepath);
		}
		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();
		return buffer;
	}
	auto Pipeline::createGraphicsPipeline(
		const std::string& vertFilepath,
		const std::string& fragFilepath,
		const PipelineConfigInfo& config
	) -> void {
		auto vertCode = this->readFile(vertFilepath);
		auto fragCode = this->readFile(fragFilepath);

		assert(
			config.pipelineLayout != VK_NULL_HANDLE &&
			"Cannot create graphics pipeline:: no pipelineLayout provided in configInfo"
		);
		assert(
			config.renderPass != VK_NULL_HANDLE &&
			"Cannot create graphics pipeline:: no renderPass provided in configInfo"
		);
		this->createShaderModule(vertCode, &this->vertShaderModule);
		this->createShaderModule(fragCode, &this->fragShaderModule);

		VkPipelineShaderStageCreateInfo shaderStages[2];

		// setup vertex shader stage
		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;							// for vertex shader
		shaderStages[0].module = this->vertShaderModule;							// shader module
		shaderStages[0].pName = "main";												// name of entry function in shader
		shaderStages[0].flags = 0;													// unused
		shaderStages[0].pNext = nullptr;											// for customizing shader functionality, unused								
		shaderStages[0].pSpecializationInfo = nullptr;								// for customizing shader functionality, unused	

		// setup fragment shader stage
		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;						// for fragment shader
		shaderStages[1].module = this->fragShaderModule;							// shader module
		shaderStages[1].pName = "main";												// name of entry function in shader
		shaderStages[1].flags = 0;													// unused
		shaderStages[1].pNext = nullptr;											// for customizing shader functionality, unused	
		shaderStages[1].pSpecializationInfo = nullptr;								// for customizing shader functionality, unused	

		auto bindingDescriptions = Model::Vertex::getBindingDescriptions();
		auto attributeDescriptions = Model::Vertex::getAttributeDescriptions();
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

		VkPipelineViewportStateCreateInfo viewportInfo{};
		viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.viewportCount = 1;												// can have multiple viewports & scissors
		viewportInfo.pViewports = &config.viewport;
		viewportInfo.scissorCount = 1;												// but only have one for now
		viewportInfo.pScissors = &config.scissor;

		// create pipeline object and connect to config
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &config.inputAssemblyInfo;
		pipelineInfo.pViewportState = &viewportInfo;
		pipelineInfo.pRasterizationState = &config.rasterizationInfo;
		pipelineInfo.pMultisampleState = &config.multisampleInfo;
		pipelineInfo.pColorBlendState = &config.colorBlendInfo;
		pipelineInfo.pDepthStencilState = &config.depthStencilInfo;
		pipelineInfo.pDynamicState = nullptr;

		pipelineInfo.layout = config.pipelineLayout;
		pipelineInfo.renderPass = config.renderPass;
		pipelineInfo.subpass = config.subpass;

		pipelineInfo.basePipelineIndex = -1;				// used for performance, allows creation of pipeline by derivation of
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;	// an existing pipeline on the gpu

		if (
			vkCreateGraphicsPipelines(
				this->device.device(),
				VK_NULL_HANDLE,				// pipeline cache, can have better performance
				1,							// single pipeline
				&pipelineInfo,				// 
				nullptr,					// no allocation callbacks
				&this->graphicsPipeline		// handle for this graphics pipeline
			) != VK_SUCCESS
		) {
			throw std::runtime_error("failed to create graphics pipeline");
		}
	}
	auto Pipeline::createShaderModule(
		const std::vector<char>& code,
		VkShaderModule* shaderModule
	) -> void {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data()); // default allocator in std::vector handles size mismatch issue

		if (vkCreateShaderModule(this->device.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module.");
		}
	}

	auto Pipeline::bind(VkCommandBuffer commandBuffer) -> void {
		vkCmdBindPipeline(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,		// VK_PIPELINE_BIND_POINT_COMPUTE & VK_PIPELINE_BIND_POINT_RAY_TRACING
			this->graphicsPipeline
		);
	}
	auto Pipeline::defaultPipelineConfigInfo(uint32_t width, uint32_t height) -> PipelineConfigInfo {
		PipelineConfigInfo configInfo{};

		configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;						// group every 3 verticies into triangle (triangle strip is a notable option)
		configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;										// allows inclusion of seperator when using triangle strip to break strip

		configInfo.viewport.x = 0.0f;								// shape of output viewport
		configInfo.viewport.y = 0.0f;
		configInfo.viewport.width = static_cast<float>(width);		// if monitor resolution is 16:9, but user wants 4:3 game res, could multiply by constants here to achieve this?
		configInfo.viewport.height = static_cast<float>(height);	// can multiply height and width by constants to stretch and squish
		configInfo.viewport.minDepth = 0.0f;
		configInfo.viewport.maxDepth = 1.0f;						// z axis

		configInfo.scissor.offset = { 0, 0 };						// anything outside is ignored and not rendered (same as viewport.x, .y
		configInfo.scissor.extent = { width, height };				// same as width, height => renders full image

		configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;			// if enabled, clamps all Z values to 0-1 (ie, something with -z will be visible)
		configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;	// discards all primitives before rasterization, but we want those
		configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;	// what to draw? just corners, edges? we want fill
		configInfo.rasterizationInfo.lineWidth = 1.0f;						
		configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;			// which face of triangle to ignore? front? back? ignore none? usually want to do back-face culling for performance
		configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;	// winding order (ie declaring normal dir, as 3 points don't tells which face is forward)
		configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;			// 
		configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f;
		configInfo.rasterizationInfo.depthBiasClamp = 0.0f;
		configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;

		configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO; 
		configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;					// at each fragment, how many times to sample?
		configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// set higher than 1 => MSAA (multi-sample anti-aliasing)?
		configInfo.multisampleInfo.minSampleShading = 1.0f;
		configInfo.multisampleInfo.pSampleMask = nullptr;
		configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;
		configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;

		configInfo.colorBlendAttachement.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		configInfo.colorBlendAttachement.blendEnable = VK_FALSE; // if color already in buffer, how do they mix? false => overwrite, need for transparancy
		configInfo.colorBlendAttachement.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		configInfo.colorBlendAttachement.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		configInfo.colorBlendAttachement.colorBlendOp = VK_BLEND_OP_ADD; // additive blending
		configInfo.colorBlendAttachement.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		configInfo.colorBlendAttachement.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		configInfo.colorBlendAttachement.alphaBlendOp = VK_BLEND_OP_ADD; // additive alpha blending

		configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
		configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
		configInfo.colorBlendInfo.attachmentCount = 1;
		configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachement;
		configInfo.colorBlendInfo.blendConstants[0] = 0.0f;
		configInfo.colorBlendInfo.blendConstants[1] = 0.0f;
		configInfo.colorBlendInfo.blendConstants[2] = 0.0f;
		configInfo.colorBlendInfo.blendConstants[3] = 0.0f;

		configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;		// store depth values
		configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;	// how to compare 2 depth values. pick smallest in this case => closest visible, behind overwritten in buffer
		configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;		// avoids drawing stuff entirely behind closer stuff, but also doesn't draw the portions of things that are behind stuff (ie, a cloud behind a mountain, part of cloud might be visible, only because the mountain doesn't cover it)
		configInfo.depthStencilInfo.minDepthBounds = 0.0f;
		configInfo.depthStencilInfo.maxDepthBounds = 1.0f;
		configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
		configInfo.depthStencilInfo.front = {};
		configInfo.depthStencilInfo.back = {};

		return configInfo;
	}
}
