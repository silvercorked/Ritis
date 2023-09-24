#pragma once

#include "Window.hpp"
#include "Pipeline.hpp"
#include "GameObject.hpp"
#include "Device.hpp"
#include "SwapChain.hpp"

#define GLM_FORCE_RADIANS					// functions expect radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// Depth buffer values will range from 0 to 1, not -1 to 1
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <memory>
#include <vector>
#include <stdexcept>
#include <array>

namespace engine {

	auto nextSierpinski(const std::vector<Model::Vertex>& prev) {
		std::vector<Model::Vertex> next = {};
		for (auto i = 0; i < prev.size(); i += 3) { // prev[i][i + 1] and [i + 2] hold the triangl's verticies
			glm::vec2 avg0_1 = (prev[i].position + prev[i + 1].position) / 2.0f; // mid-point between vertex 0 and 1
			glm::vec2 avg0_2 = (prev[i].position + prev[i + 2].position) / 2.0f; // mid-point between vertex 0 and 2
			glm::vec2 avg1_2 = (prev[i + 1].position + prev[i + 2].position) / 2.0f; // mid-point between vertex 1 and 2
			{	// first triangle (0th vertex + 01 midpoint + 02 midpoint)
				next.push_back({ prev[i].position , prev[i].color });
				next.push_back({ avg0_1, prev[i + 1].color });
				next.push_back({ avg0_2, prev[i + 2].color });
			}
			{	// second (1st vertex + 01 midpoint + 12 midpoint)
				next.push_back({ prev[i + 1].position , prev[i + 1].color });
				next.push_back({ avg0_1, prev[i].color });
				next.push_back({ avg1_2, prev[i + 2].color });
			}
			{	// third (2nd vertex + 02 midpoint + 12 midpoint)
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
		std::vector<GameObject> gameObjects;

		auto loadGameObjects() -> void;
		auto createPipelineLayout() -> void;
		auto createPipeline() -> void;
		auto createCommandBuffers() -> void;
		auto freeCommandBuffers() -> void;
		auto drawFrame() -> void;
		auto recreateSwapChain() -> void;
		auto recordCommandBuffer(int) -> void;
		auto renderGameObjects(VkCommandBuffer) -> void;
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
		this->loadGameObjects();
		this->createPipelineLayout();
		this->recreateSwapChain(); // calls create pipeline
		this->createCommandBuffers();
	}
	FirstApp::~FirstApp() {
		vkDestroyPipelineLayout(this->device.device(), this->pipelineLayout, nullptr);
	}

	auto FirstApp::loadGameObjects() -> void {
		std::vector<Model::Vertex> verticies {
			{ {0.0f, -0.5f}, { 1.0f, 0.0f, 0.0f } },
			{ {0.5f, 0.5f}, {0.0f, 1.0f, 0.0f} },
			{ {-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f} }
		};
		//const auto SeirpinskiDepth = 2;
		//for (auto i = 0; i < SeirpinskiDepth; i++)
		//	verticies = nextSierpinski(verticies);
		auto model = std::make_shared<Model>(this->device, verticies);

		auto triangle = GameObject::createGameObject();
		triangle.model = model;
		triangle.color = { 0.1f, 0.8f, 0.1f };
		//triangle.transform2d.translation.x = 0.2f;
		triangle.transform2d.scale = { 1.0f, 1.0f };
		triangle.transform2d.rotation = 0;//.25f * glm::two_pi<float>();

		this->gameObjects.push_back(std::move(triangle));
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

		this->renderGameObjects(commandBuffers[imageIndex]);
		

		vkCmdEndRenderPass(this->commandBuffers[imageIndex]); // call End event to transition from Recording to Executable
		if (vkEndCommandBuffer(this->commandBuffers[imageIndex]) != VK_SUCCESS) {
			throw std::runtime_error("failed to transition command buffer to executable state");
		}
	}
	auto FirstApp::renderGameObjects(VkCommandBuffer commandBuffer) -> void {
		static int i = 0;
		i++;
		static float r = 2.9f;
		if (i == 1000) {
			i = 0;
			r += 0.01f;
		}
		if (r > 4.0f) r = 2.9f;
		static float prevRotationRate = 0.01; // /100
		this->pipeline->bind(commandBuffer);

		for (auto& obj : this->gameObjects) {
			//obj.transform2d.rotation = glm::mod(obj.transform2d.rotation + 0.0001f, glm::two_pi<float>());
			const float prevInput = prevRotationRate * 50;
			// start point is 0.5 cause 0 is a sink
			std::cout << "prevInput: " << prevInput << " r: " << r << std::endl;
			const float nextInput = prevInput * r * (1 - prevInput);
			const float nextRotationRate = nextInput / 50;
			if (i == 200 || i == 305 || i == 402 || i == 507 || i == 604 || i == 701 || i == 806 || i == 999) {
				obj.transform2d.rotation = glm::mod<float>(
					obj.transform2d.rotation + nextRotationRate, glm::two_pi<float>()
				);
			}
			prevRotationRate = nextRotationRate;
		}

		for (auto& obj : this->gameObjects) {
			SimplePushConstantData push{};
			push.offset = obj.transform2d.translation;
			push.color = obj.color;
			push.transform = obj.transform2d.mat2();

			vkCmdPushConstants(
				commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push
			);
			obj.model->bind(commandBuffer);
			obj.model->draw(commandBuffer);
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
