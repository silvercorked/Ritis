#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <iostream>

#include "Device.hpp"

namespace engine {
	struct PipelineConfigInfo {};
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
		~Pipeline() {};
		Pipeline(const Pipeline&) = delete;
		auto operator=(const Pipeline&) = delete;

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

		std::cout << "Vertex Shader Code Size: " << vertCode.size() << std::endl;
		std::cout << "Fragment Shader Code Size: " << fragCode.size() << std::endl;
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

	auto Pipeline::defaultPipelineConfigInfo(uint32_t width, uint32_t height) -> PipelineConfigInfo {
		PipelineConfigInfo configInfo{};

		return configInfo;
	}
}
