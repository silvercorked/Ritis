#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <iostream>

namespace engine {
	class Pipeline {
		static auto readFile(const std::string& filepath) -> std::vector<char>;
		auto createGraphicsPipeline(const std::string& vertFilepath, const std::string& fragFilepath) -> void;
	public:
		Pipeline(const std::string& vertFilepath, const std::string& fragFilepath);
	};

	Pipeline::Pipeline(const std::string& vertFilepath, const std::string& fragFilepath) {
		createGraphicsPipeline(vertFilepath, fragFilepath);
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
	auto Pipeline::createGraphicsPipeline(const std::string& vertFilepath, const std::string& fragFilepath) -> void {
		auto vertCode = this->readFile(vertFilepath);
		auto fragCode = this->readFile(fragFilepath);

		std::cout << "Vertex Shader Code Size: " << vertCode.size() << std::endl;
		std::cout << "Fragment Shader Code Size: " << fragCode.size() << std::endl;
	}
}
