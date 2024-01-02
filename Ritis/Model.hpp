#pragma once

#include "Device.hpp"
#include "Buffer.hpp"
#include "Utils.hpp"

#define GLM_FORCE_RADIANS					// functions expect radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// Depth buffer values will range from 0 to 1, not -1 to 1
#include <glm/glm.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <vector>
#include <cassert>
#include <memory>
#include <iostream>
#include <unordered_map>

namespace engine {
	/*
		Take vertex data from cpu, allocate memory and copy data over to device gpu
	*/
	class Model {
		Device& device;

		std::unique_ptr<Buffer> vertexBuffer;
		uint32_t vertexCount;
		
		bool hasIndexBuffer = false;			// optional index buffer support. just leave input builder's indices member empty if not desired
		std::unique_ptr<Buffer> indexBuffer;	// this allows the vertex buffer to only contain unique vertices (and any associated data, like color) // index buffers list indices of vertices in the vertex buffer
		uint32_t indexCount;					// and avoids copying vertex data to form each triangle

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
			glm::vec3 normal{};
			glm::vec2 uv{};
			
			static auto getBindingDescriptions() -> std::vector<VkVertexInputBindingDescription>;
			static auto getAttributeDescriptions() -> std::vector<VkVertexInputAttributeDescription>;

			bool operator==(const Vertex& other) const {
				return this->position == other.position
					&& this->color == other.color
					&& this->normal == other.normal
					&& this->uv == other.uv;
			}
		};

		struct Builder {
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};

			auto loadModel(const std::string& filepath) -> void;
		};

		Model(Device& device, const Model::Builder& builder);
		~Model();

		Model(const Model&) = delete;
		Model& operator=(const Model&) = delete;

		static auto createModelFromFile(Device& device, const std::string& filepath) -> std::unique_ptr<Model>;

		auto bind(VkCommandBuffer commandBuffer) -> void;
		auto draw(VkCommandBuffer commandBuffer) -> void;

	private:
		auto createVertexBuffers(const std::vector<Vertex>& vertices) -> void;
		auto createIndexBuffers(const std::vector<uint32_t>& indices) -> void;
	};
}

namespace std { // p sure this is undefined behavior
	template<>
	struct hash<engine::Model::Vertex> {
		size_t operator()(engine::Model::Vertex const& vertex) const {
			size_t seed = 0;
			engine::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
			return seed;
		}
	};
}

namespace engine {
	Model::Model(Device& d, const Model::Builder& builder) :
		device{ d }
	{
		this->createVertexBuffers(builder.vertices);
		this->createIndexBuffers(builder.indices);
	}
	Model::~Model() {}

	auto Model::createModelFromFile(Device& device, const std::string& filepath) -> std::unique_ptr<Model> {
		Builder builder{};
		builder.loadModel(filepath);
		std::cout << "Vertex count: " << builder.vertices.size() << "\n";
		return std::make_unique<Model>(device, builder);
	}

	auto Model::createVertexBuffers(const std::vector<Vertex>& vertices) -> void {
		this->vertexCount = static_cast<uint32_t>(vertices.size());
		assert(this->vertexCount >= 3 && "Vertex count must be at least 3");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * this->vertexCount;
		uint32_t vertexSize = sizeof(vertices[0]);

		Buffer stagingBuffer{
			this->device,
			vertexSize,
			this->vertexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,			// buffer will be used as a source location for a memory transfer on device
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT			// host = CPU (device = GPU), so cpu accessible. Needed to write from cpu. host changes affect device side. 
		};

		stagingBuffer.map(); // map stagingBufferMemory to void* data 
		stagingBuffer.writeToBuffer((void*)vertices.data()); // vertices written to host memory. coherent bit flushed changes
		// end mapping (happens during cleanup), now do internal transfer to device only memory for faster operation (as it doesnt need to check if things have been written to it)

		this->vertexBuffer = std::make_unique<Buffer>(
			this->device,
			vertexSize,
			this->vertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,	// buffer will be used to hold vertex buffer data and be a destination from a staging buffer
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT // optimized device only memory
		);

		this->device.copyBuffer(stagingBuffer.getBuffer(), this->vertexBuffer->getBuffer(), bufferSize); // copy host & device side staging buffer into device only vertex buffer
		// cleanup buffers
	}
	auto Model::createIndexBuffers(const std::vector<uint32_t>& indices) -> void {	// similar to vertex buffer
		this->indexCount = static_cast<uint32_t>(indices.size());
		this->hasIndexBuffer = this->indexCount > 0;
		if (!this->hasIndexBuffer) return;											// can early return
		VkDeviceSize bufferSize = sizeof(indices[0]) * this->indexCount;
		uint32_t indexSize = sizeof(indices[0]);

		Buffer stagingBuffer{
			this->device,
			indexSize,
			this->indexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*) indices.data());

		this->indexBuffer = std::make_unique<Buffer>(
			this->device,
			indexSize,
			this->indexCount,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, // index buffer but also destination for staging buffer copy
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT // optimized device only memory
		);

		this->device.copyBuffer(stagingBuffer.getBuffer(), this->indexBuffer->getBuffer(), bufferSize);
	}

	auto Model::bind(VkCommandBuffer commandBuffer) -> void {
		VkBuffer buffers[] = { this->vertexBuffer->getBuffer()};
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers( // record to command buffer to bind 1 vertex buffers starting at binding 0 with offset 0 into buffer
			commandBuffer,
			0,
			1,
			buffers,
			offsets
		);

		if (this->hasIndexBuffer) {
			vkCmdBindIndexBuffer(commandBuffer, this->indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32); // could use different int type here to save space or make room for more vertices
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
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		attributeDescriptions.push_back({	// position
			0,							// location
			0,							// binding // interleaving, so binding same
			VK_FORMAT_R32G32B32_SFLOAT,	// format
			offsetof(Vertex, position)	// offset
		});
		attributeDescriptions.push_back({	// color
			1,							// location
			0,							// binding
			VK_FORMAT_R32G32B32_SFLOAT,	// format
			offsetof(Vertex, color)		// offset
		});
		attributeDescriptions.push_back({	// normal
			2,							// location
			0,							// binding
			VK_FORMAT_R32G32B32_SFLOAT,	// format
			offsetof(Vertex, normal)	// offset
		});
		attributeDescriptions.push_back({	// uv
			3,							// location
			0,							// binding
			VK_FORMAT_R32G32_SFLOAT,	// format
			offsetof(Vertex, uv)		// offset
		});
		return attributeDescriptions;
	}

	auto Model::Builder::loadModel(const std::string& filepath) -> void {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str()))
			throw std::runtime_error(warn + err);

		this->vertices.clear();
		this->indices.clear();

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};
		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};
				if (index.vertex_index >= 0) {
					vertex.position = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2]
					};
					vertex.color = {
						attrib.colors[3 * index.vertex_index + 0],
						attrib.colors[3 * index.vertex_index + 1],
						attrib.colors[3 * index.vertex_index + 2]
					};
				}
				if (index.normal_index >= 0) {
					vertex.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2]
					};
				}
				if (index.texcoord_index >= 0) {
					vertex.uv = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						attrib.texcoords[2 * index.texcoord_index + 1]
					};
				}
				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(this->vertices.size());
					this->vertices.push_back(vertex);
				}
				this->indices.push_back(uniqueVertices[vertex]);
			}
		}
	}
}
