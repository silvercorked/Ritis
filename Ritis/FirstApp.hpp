#pragma once

#include "Window.hpp"
#include "Pipeline.hpp"
#include "Device.hpp"
#include "SwapChain.hpp"
#include "Model.hpp"

// std
#include <memory>
#include <vector>
#include <stdexcept>
#include <array>

namespace engine {

	auto nextSierpinski(const std::vector<Model::Vertex>& prev) {
		std::vector<Model::Vertex> next = {};
		for (auto i = 0; i < prev.size(); i += 3) { // prev[i][i + 1] and [i + 2] hold the triangl's verticies
			Model::Vertex avg0_1 = { (prev[i].position + prev[i + 1].position) / 2.0f };
			Model::Vertex avg0_2 = { (prev[i].position + prev[i + 2].position) / 2.0f };
			Model::Vertex avg1_2 = { (prev[i + 1].position + prev[i + 2].position) / 2.0f };
			{	// first triangle
				next.push_back({ prev[i].position });
				next.push_back(avg0_1);
				next.push_back(avg0_2);
			}
			{	// second
				next.push_back({ prev[i + 1].position });
				next.push_back(avg0_1);
				next.push_back(avg1_2);
			}
			{	// third
				next.push_back({ prev[i + 2].position });
				next.push_back(avg0_2);
				next.push_back(avg1_2);
			}
		}
		return next;
	}

	class FirstApp {
		Window window{ WIDTH, HEIGHT, "Hello, World!" };
		Device device{ window };
		SwapChain swapChain{ device, window.getExtent() };
		std::unique_ptr<Pipeline> pipeline;
		VkPipelineLayout pipelineLayout;
		std::vector<VkCommandBuffer> commandBuffers;
		std::unique_ptr<Model> model;

		auto loadModels() -> void;
		auto createPipelineLayout() -> void;
		auto createPipeline() -> void;
		auto createCommandBuffers() -> void;
		auto drawFrame() -> void;
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
		this->createPipeline();
		this->createCommandBuffers();
	}
	FirstApp::~FirstApp() {
		vkDestroyPipelineLayout(this->device.device(), this->pipelineLayout, nullptr);
	}

	auto FirstApp::loadModels() -> void {
		std::vector<Model::Vertex> verticies {
			{ {0.0f, -0.95f} },
			{ {0.95f, 0.95f} },
			{ {-0.8f, 0.5f} }
		};
		const auto SeirpinskiDepth = 7;
		for (auto i = 0; i < SeirpinskiDepth; i++)
			verticies = nextSierpinski(verticies);
		this->model = std::make_unique<Model>(this->device, verticies);
	}

	auto FirstApp::createPipelineLayout() -> void {
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		pipelineLayoutInfo.setLayoutCount = 0;						// empty layout, layouts can be used to send additional data
		pipelineLayoutInfo.pSetLayouts = nullptr;					// to vertex and fragment shaders (textures, uniform buffer objs, .etc)

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
	auto FirstApp::createPipeline() -> void {
		// use swapchain width and height because they don't necessarily match the window
		auto pipelineConfig = Pipeline::defaultPipelineConfigInfo(swapChain.width(), swapChain.height());

		pipelineConfig.renderPass = swapChain.getRenderPass(); // render pass describes structure and format of frame buffer objects
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
		this->commandBuffers.resize(this->swapChain.imageCount()); // one to one to avoid re-Recording command buffers as they target a frame buffer
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

		for (auto i = 0; i < this->commandBuffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			if (
				vkBeginCommandBuffer( // call Begin event to transition from Initial to Recording
					this->commandBuffers.at(i),
					&beginInfo
				) != VK_SUCCESS
			) {
				throw std::runtime_error("failed to transition command buffer to recording state");
			}

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = this->swapChain.getRenderPass();
			renderPassInfo.framebuffer = this->swapChain.getFrameBuffer(i); // associate with frame buffer

			/*
				framebuffers have 2 parts so far, color (index 0) and depth (index 1). This is according to structure given in renderPass
			*/

			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = this->swapChain.getSwapChainExtent();

			std::array<VkClearValue, 2> clearValues{};
			clearValues[0].color = { 0.1f, 0.1f, 0.1f, 0.1f };
			clearValues[1].depthStencil = { 1.0f, 0 };
			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(
				this->commandBuffers[i],
				&renderPassInfo,				// record render pass to command buffer
				VK_SUBPASS_CONTENTS_INLINE		// only a primary command buffer will be used, no secondaries
			);									// VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS if ^ isn't the case
			// can't have a command buffer that has inline commands and secondary command buffers

			this->pipeline->bind(this->commandBuffers[i]);
			this->model->bind(commandBuffers[i]);
			this->model->draw(commandBuffers[i]);

			vkCmdEndRenderPass(this->commandBuffers[i]); // call End event to transition from Recording to Executable
			if (vkEndCommandBuffer(this->commandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to transition command buffer to executable state");
			}
		}
	}
	auto FirstApp::drawFrame() -> void {
		uint32_t imageIndex;																// holds which buffer set we're using
		auto result = this->swapChain.acquireNextImage(&imageIndex);

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image");
		}

		result = this->swapChain.submitCommandBuffers(&this->commandBuffers[imageIndex], &imageIndex);
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
