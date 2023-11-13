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
		
		bool hasIndexBuffer = false;		// optional index buffer support. just leave input builder's indices member empty if not desired
		VkBuffer indexBuffer;				// index buffers list indices of vertices in the vertex buffer
		VkDeviceMemory indexBufferMemory;	// this allows the vertex buffer to only contain unique vertices (and any associated data, like color)
		uint32_t indexCount;				// and avoids copying vertex data to form each triangle

		/*
		 v1 ______ v2/v4
			|   /|
			|  / |
			| /  |
	  v3/v5 |/___| v6
			vertexBuffer (no indexbuffer) = {v1, v2, v3, v4, v5, v6}

			vertexBuffer (w/ indexbuffer) = {v1, v2, v3, v6}
			indexBuffer = {0, 1, 2, 1, 2, 3}
			0, 1, 2 => v1, v2, v3. 1, 2, 3 => v2, v3, v6.

			With complex models, it's feasible to have tons of shared vertices. Index buffers reduce memory usage as models get more complex
		*/

	public:
		struct Vertex {
			glm::vec3 position;
			glm::vec3 color;
			
			static auto getBindingDescriptions() -> std::vector<VkVertexInputBindingDescription>;
			static auto getAttributeDescriptions() -> std::vector<VkVertexInputAttributeDescription>;
		};

		struct Builder {
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};
		};

		Model(Device& device, const Model::Builder& builder);
		~Model();

		Model(const Model&) = delete;
		Model& operator=(const Model&) = delete;

		auto bind(VkCommandBuffer commandBuffer) -> void;
		auto draw(VkCommandBuffer commandBuffer) -> void;

	private:
		auto createVertexBuffers(const std::vector<Vertex>& vertices) -> void;
		auto createIndexBuffers(const std::vector<uint32_t>& indices) -> void;
	};

	Model::Model(Device& d, const Model::Builder& builder) :
		device{d}
	{
		this->createVertexBuffers(builder.vertices);
		this->createIndexBuffers(builder.indices);
	}
	Model::~Model() {
		// passing in nullptr when dealing with memory isn't efficient. will change later
		vkDestroyBuffer(device.device(), this->vertexBuffer, nullptr);
		vkFreeMemory(this->device.device(), this->vertexBufferMemory, nullptr);
		if (this->hasIndexBuffer) {
			vkDestroyBuffer(device.device(), this->indexBuffer, nullptr);
			vkFreeMemory(this->device.device(), this->indexBufferMemory, nullptr);
		}
	}

	auto Model::createVertexBuffers(const std::vector<Vertex>& vertices) -> void {
		this->vertexCount = static_cast<uint32_t>(vertices.size());
		assert(this->vertexCount >= 3 && "Vertex count must be at least 3");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * this->vertexCount;
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		this->device.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,			// buffer will be used as a source location for a memory transfer on device
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT			// host = CPU (device = GPU), so cpu accessible. Needed to write from cpu. host changes affect device side. 
			| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);
		void* data;
		vkMapMemory(this->device.device(), stagingBufferMemory, 0, bufferSize, 0, &data); // map stagingBufferMemory to void* data
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));	// vertices written to host memory. coherent bit flushed changes
		vkUnmapMemory(this->device.device(), stagingBufferMemory);		// end mapping, now do internal transfer to device only memory for faster operation (as it doesnt need to check if things have been written to it)

		this->device.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,	// buffer will be used to hold vertex buffer data and be a destination from a staging buffer
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // optimized device only memory
			this->vertexBuffer,
			this->vertexBufferMemory
		);
		this->device.copyBuffer(stagingBuffer, this->vertexBuffer, bufferSize); // copy host & device side staging buffer into device only vertex buffer
		vkDestroyBuffer(this->device.device(), stagingBuffer, nullptr);
		vkFreeMemory(this->device.device(), stagingBufferMemory, nullptr);
	}
	auto Model::createIndexBuffers(const std::vector<uint32_t>& indices) -> void {	// similar to vertex buffer
		this->indexCount = static_cast<uint32_t>(indices.size());
		this->hasIndexBuffer = this->indexCount > 0;
		if (!this->hasIndexBuffer) return;											// can early return
		VkDeviceSize bufferSize = sizeof(indices[0]) * this->indexCount;
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		this->device.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);
		void* data;
		vkMapMemory(this->device.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(this->device.device(), stagingBufferMemory);

		this->device.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, // index buffer but also destination for staging buffer copy
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // optimized device only memory
			this->indexBuffer,
			this->indexBufferMemory
		);
		this->device.copyBuffer(stagingBuffer, this->indexBuffer, bufferSize);
		vkDestroyBuffer(this->device.device(), stagingBuffer, nullptr);
		vkFreeMemory(this->device.device(), stagingBufferMemory, nullptr);
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

		if (this->hasIndexBuffer) {
			vkCmdBindIndexBuffer(commandBuffer, this->indexBuffer, 0, VK_INDEX_TYPE_UINT32); // could use different int type here to save space or make room for more vertices
		}
	}
	auto Model::draw(VkCommandBuffer commandBuffer) -> void {
		if (this->hasIndexBuffer) {
			vkCmdDrawIndexed(
				commandBuffer,
				this->indexCount,
				1,
				0,
				0,
				0
			);
		}
		else {
			vkCmdDraw(
				commandBuffer,
				vertexCount,
				1,
				0,
				0
			);
		}
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
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, position); // useful macro to know

		attributeDescriptions[1].binding = 0; // interleaving, so binding same
		attributeDescriptions[1].location = 1; // location = 1
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);
		return attributeDescriptions;
	}
}
