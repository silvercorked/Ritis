#pragma once

#include "Window.hpp"
#include "Pipeline.hpp"
#include "Device.hpp"
#include "SwapChain.hpp"
#include "Model.hpp"

#define GLM_FORCE_RADIANS					// functions expect radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// Depth buffer values will range from 0 to 1, not -1 to 1
#include <glm/glm.hpp>

// std
#include <memory>
#include <vector>
#include <stdexcept>
#include <array>

namespace engine {

	auto nextSierpinski(const std::vector<Model::Vertex>& prev) {
		std::vector<Model::Vertex> next = {};
		for (auto i = 0; i < prev.size(); i += 3) { // prev[i][i + 1] and [i + 2] hold the triangl's verticies
			glm::vec2 avg0_1 = (prev[i].position + prev[i + 1].position) / 2.0f;
			glm::vec2 avg0_2 = (prev[i].position + prev[i + 2].position) / 2.0f;
			glm::vec2 avg1_2 = (prev[i + 1].position + prev[i + 2].position) / 2.0f;
			{	// first triangle
				next.push_back({ prev[i].position , prev[i].color });
				next.push_back({ avg0_1, prev[i + 1].color });
				next.push_back({ avg0_2, prev[i + 2].color });
			}
			{	// second
				next.push_back({ prev[i + 1].position , prev[i + 1].color });
				next.push_back({ avg0_1, prev[i].color });
				next.push_back({ avg1_2, prev[i + 2].color });
			}
			{	// third
				next.push_back({ prev[i + 2].position , prev[i + 2].color });
				next.push_back({ avg0_2, prev[i].color });
				next.push_back({ avg1_2, prev[i + 1].color });
			}
		}
		return next;
	}

	// push constants have a particular memory layout,
	// vec3 must be aligned to a multiple of 16 by vulkan docs
	// vec2 takes up 8, so pad 8 more then place vec 3
	struct SimplePushConstantData {
		glm::mat2 transform{1.0f};
		glm::vec2 offset;
		alignas(16) glm::vec3 color;
	};

	class FirstApp {
		Window window{ WIDTH, HEIGHT, "Hello, World!" };
		Device device{ window };
		std::unique_ptr<SwapChain> swapChain;
		std::unique_ptr<Pipeline> pipeline;
		VkPipelineLayout pipelineLayout;
		std::vector<VkCommandBuffer> commandBuffers;
		std::unique_ptr<Model> model;

		auto loadModels() -> void;
		auto createPipelineLayout() -> void;
		auto createPipeline() -> void;
		auto createCommandBuffers() -> void;
		auto freeCommandBuffers() -> void;
		auto drawFrame() -> void;
		auto recreateSwapChain() -> void;
		auto recordCommandBuffer(int) -> void;
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;
		
		FirstApp();
		~FirstApp();

		FirstApp(const FirstApp&) = delete;
		FirstApp& operator=(const FirstApp&) = delete;

		auto run() -> void;
	};

	FirstApp::FirstApp() {
		this->loadModels();
		this->createPipelineLayout();
		this->recreateSwapChain(); // calls create pipeline
		this->createCommandBuffers();
	}
	FirstApp::~FirstApp() {
		vkDestroyPipelineLayout(this->device.device(), this->pipelineLayout, nullptr);
	}

	auto FirstApp::loadModels() -> void {
		std::vector<Model::Vertex> verticies {
			{ {0.0f, -0.3f}, {1.0f, 0.0f, 0.0f} },
			{ {0.2f, -0.1f}, {0.0f, 1.0f, 0.0f} },
			{ {-0.2f, -0.1f}, {0.0f, 0.0f, 1.0f} }
		};
		//const auto SeirpinskiDepth = 2;
		//for (auto i = 0; i < SeirpinskiDepth; i++)
		//	verticies = nextSierpinski(verticies);
		this->model = std::make_unique<Model>(this->device, verticies);
	}

	auto FirstApp::createPipelineLayout() -> void {
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
	auto FirstApp::recreateSwapChain() -> void {
		auto extent = this->window.getExtent();
		while (extent.width == 0 || extent.height == 0) {	// if one dimension is sizeless
			extent = this->window.getExtent();
			glfwWaitEvents();								// pause and wait
		}
		vkDeviceWaitIdle(this->device.device());	// wait swapchain to be idle
		if (this->swapChain == nullptr)
			this->swapChain = std::make_unique<SwapChain>(this->device, extent);
		else {
			this->swapChain = std::make_unique <SwapChain>(this->device, extent, std::move(this->swapChain));
			if (this->swapChain->imageCount() != commandBuffers.size()) {
				freeCommandBuffers();
				createCommandBuffers();
			}
		}
		this->createPipeline(); // pipeline depends on swapchain renderpass
		// if renderpass compatible, can remove avoid recreating pipeline
	}
	auto FirstApp::createPipeline() -> void {
		assert(this->swapChain != nullptr && "Cannot create pipeline before swap chain");
		assert(this->pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
		// use swapchain width and height because they don't necessarily match the window
		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = this->swapChain->getRenderPass(); // render pass describes structure and format of frame buffer objects
		pipelineConfig.pipelineLayout = this->pipelineLayout;
		this->pipeline = std::make_unique<Pipeline>(
			device,
			"shaders/simpleShader.vert.spv",
			"shaders/simpleShader.frag.spv",
			pipelineConfig
		);
	}
	auto FirstApp::createCommandBuffers() -> void {
		/*
			Command buffers hold commands for the api
			Pros:
				- often, not much changes in terms of what needs to be done each frame, so this can be created
					once and then reused every frame.

			Lifecycle State Diagram:
				(State) on (Event) -> (next State)
				Initial on Begin -> Recording
				Recording on Reset -> Initial
				Recording on Invalidate -> Invalid
				Recording on End -> Executable
				Executable on Reset -> Initial
				Executable on Invalidate -> Invalid
				Executable on Submission -> Pending
				Pending on OneTimeSubmit -> Invalid
				Pending on Completion -> Executable
				Invalid on Reset -> Initial

			*note: undefined behavior to call Submit on a Command Buffer in the Pending State
		*/
		this->commandBuffers.resize(this->swapChain->imageCount()); // one to one to avoid re-Recording command buffers as they target a frame buffer
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;					// primary: can be submitted to device for submission, can call secondary command buffers
																			// secondary: cannot be submitted to device for submission, but can be called by primary command buffers
		allocInfo.commandPool = this->device.getCommandPool(); // avoid allocation and deallocating command buffer memory using command pools
		allocInfo.commandBufferCount = static_cast<uint32_t>(this->commandBuffers.size());

		if (
			vkAllocateCommandBuffers(
				this->device.device(),
				&allocInfo,
				this->commandBuffers.data()
			) != VK_SUCCESS
		) {
			throw std::runtime_error("Failed to allocate command buffers");
		}
	}
	auto FirstApp::freeCommandBuffers() -> void {
		vkFreeCommandBuffers(
			this->device.device(),
			this->device.getCommandPool(),
			static_cast<uint32_t>(this->commandBuffers.size()),
			this->commandBuffers.data()
		);
		this->commandBuffers.clear();
	}
	auto FirstApp::recordCommandBuffer(int imageIndex) -> void {
		static int frame = 0;
		frame = (frame + 1) % 10000;

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (
			vkBeginCommandBuffer( // call Begin event to transition from Initial to Recording
				this->commandBuffers.at(imageIndex),
				&beginInfo
			) != VK_SUCCESS
			) {
			throw std::runtime_error("failed to transition command buffer to recording state");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = this->swapChain->getRenderPass();
		renderPassInfo.framebuffer = this->swapChain->getFrameBuffer(imageIndex); // associate with frame buffer

		/*
			framebuffers have 2 parts so far, color (index 0) and depth (index 1). This is according to structure given in renderPass
		*/

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = this->swapChain->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(
			this->commandBuffers[imageIndex],
			&renderPassInfo,				// record render pass to command buffer
			VK_SUBPASS_CONTENTS_INLINE		// only a primary command buffer will be used, no secondaries
		);									// VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS if ^ isn't the case
		// can't have a command buffer that has inline commands and secondary command buffers

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(this->swapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(this->swapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, this->swapChain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);		// set viewport just before executing each frame
		vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);		// set scissor just before executing each frame
		// 0 for viewport index, 1 for viewport count

		this->pipeline->bind(this->commandBuffers[imageIndex]);
		this->model->bind(commandBuffers[imageIndex]);
		for (int j = 0; j < 4; j++) {
			SimplePushConstantData push{};
			const float half = (frame - j * 150) / 5000.0f;
			const float frameXOff = cos(half * 3.14);
			const float frameYOff = sin(half * 3.14);
			push.offset = { frameXOff * 0.5f, frameYOff * 0.5f };
			push.color = { 0.0f, 0.0f, 0.2f + 0.2f * j };

			vkCmdPushConstants(
				commandBuffers[imageIndex],
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push
			);
			this->model->draw(commandBuffers[imageIndex]); // draw 4 duplicates differing on push constants
		}
		

		vkCmdEndRenderPass(this->commandBuffers[imageIndex]); // call End event to transition from Recording to Executable
		if (vkEndCommandBuffer(this->commandBuffers[imageIndex]) != VK_SUCCESS) {
			throw std::runtime_error("failed to transition command buffer to executable state");
		}
	}
	auto FirstApp::drawFrame() -> void {
		uint32_t imageIndex;																// holds which buffer set we're using
		auto result = this->swapChain->acquireNextImage(&imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {	// is thrown after window resize and in some other cases
			this->recreateSwapChain();				// requires swapchain recreation
			return;
		}
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image");
		}

		this->recordCommandBuffer(imageIndex);
		result = this->swapChain->submitCommandBuffers(&this->commandBuffers[imageIndex], &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || this->window.wasWindowResized()) {
			this->window.resetWindowResizeFlag();
			this->recreateSwapChain();
			return;
		}
		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image");
		}
	}

	auto FirstApp::run() -> void {
		while (!window.shouldClose()) {
			glfwPollEvents();
			this->drawFrame();
		}

		vkDeviceWaitIdle(this->device.device());
	}
}
