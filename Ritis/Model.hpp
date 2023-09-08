#pragma once

#include "Device.hpp"

#define GLM_FORCE_RADIANS					// functions expect radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// Depth buffer values will range from 0 to 1, not -1 to 1
#include <glm/glm.hpp>

#include <vector>
#include <cassert>

namespace engine {
	/*
		Take vertex data from cpu, allocate memory and copy data over to device gpu
	*/
	class Model {
		Device& device;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		uint32_t vertexCount;
	public:
		struct Vertex {
			glm::vec2 position;
			glm::vec3 color;
			
			static auto getBindingDescriptions() -> std::vector<VkVertexInputBindingDescription>;
			static auto getAttributeDescriptions() -> std::vector<VkVertexInputAttributeDescription>;
		};

		Model(Device& device, const std::vector<Vertex>& vertices);
		~Model();

		Model(const Model&) = delete;
		Model& operator=(const Model&) = delete;

		auto bind(VkCommandBuffer commandBuffer) -> void;
		auto draw(VkCommandBuffer commandBuffer) -> void;

	private:
		auto createVertexBuffers(const std::vector<Vertex>& vertices) -> void;
	};

	Model::Model(Device& d, const std::vector<Vertex>& vertices) :
		device{d}
	{
		this->createVertexBuffers(vertices);
	}
	Model::~Model() {
		// passing in nullptr when dealing with memory isn't efficient. will change later
		vkDestroyBuffer(device.device(), this->vertexBuffer, nullptr);
		vkFreeMemory(this->device.device(), this->vertexBufferMemory, nullptr);
	}

	auto Model::createVertexBuffers(const std::vector<Vertex>& vertices) -> void {
		vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >= 3 && "Vertex count must be at least 3");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
		this->device.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,			// buffer will be used to hold vertex buffer data
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT			// host = CPU (device = GPU), so cpu accessible. Needed to write from cpu
			| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,	// host changes affect device side. Otherwise have to do manually
			this->vertexBuffer,
			this->vertexBufferMemory
		);
		void* data;
		vkMapMemory(						// point data -> start of vertex buffer memory
			this->device.device(),
			vertexBufferMemory,
			0,
			bufferSize,
			0,
			&data
		);
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));	// vertices written to host memory. coherent bit flushed changes
		vkUnmapMemory(this->device.device(), vertexBufferMemory);		// automatically to device side vertex buffer memory
	}

	auto Model::bind(VkCommandBuffer commandBuffer) -> void {
		VkBuffer buffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers( // record to command buffer to bind 1 vertex buffers starting at binding 0 with offset 0 into buffer
			commandBuffer,
			0,
			1,
			buffers,
			offsets
		);
	}
	auto Model::draw(VkCommandBuffer commandBuffer) -> void {
		vkCmdDraw(
			commandBuffer,
			vertexCount,
			1,
			0,
			0
		);
	}

	auto Model::Vertex::getBindingDescriptions() -> std::vector<VkVertexInputBindingDescription> {
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescriptions;
	}
	auto Model::Vertex::getAttributeDescriptions() -> std::vector<VkVertexInputAttributeDescription> {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, position); // useful macro to know

		attributeDescriptions[1].binding = 0; // interleaving, so binding same
		attributeDescriptions[1].location = 1; // location = 1
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);
		return attributeDescriptions;
	}
}
