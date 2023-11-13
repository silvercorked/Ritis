#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <stdexcept>

namespace engine {
	class Window {
		GLFWwindow* window;
		int width;
		int height;
		bool framebufferResized = false;
		std::string windowName;

		static auto frambufferResizeCallback(GLFWwindow*, int, int) -> void;
		auto initWindow() -> void;
	public:
		Window(int, int, std::string);
		~Window();

		Window(const Window&) = delete;					// copy and assignment deleted
		Window& operator=(const Window&) = delete;		// for RAII

		auto shouldClose() -> bool;
		auto getExtent() -> VkExtent2D;
		auto wasWindowResized() -> bool;
		auto resetWindowResizeFlag() -> void;
		auto createWindowSurface(VkInstance, VkSurfaceKHR*) -> void;
		auto getGFLWWindow() const -> GLFWwindow*;
	};

	Window::Window(int w, int h, std::string name) :
		width(w),
		height(h),
		windowName(name)
	{
		this->initWindow();
	}
	Window::~Window() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}
	inline auto Window::shouldClose() -> bool {
		return glfwWindowShouldClose(this->window);
	}
	auto Window::getExtent() -> VkExtent2D {
		return { static_cast<uint32_t>(this->width), static_cast<uint32_t>(this->height) };
	}
	inline auto Window::wasWindowResized() -> bool {
		return framebufferResized;
	}
	inline auto Window::resetWindowResizeFlag() -> void {
		this->framebufferResized = false;
	}
	auto Window::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) -> void {
		if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface");
		}
	}
	auto Window::getGFLWWindow() const -> GLFWwindow* {
		return this->window;
	}
	auto Window::frambufferResizeCallback(GLFWwindow* currWindow, int width, int height) -> void {
		auto w = reinterpret_cast<Window*>(glfwGetWindowUserPointer(currWindow));
		w->framebufferResized = true;
		w->width = width;
		w->height = height;
	}

	auto Window::initWindow() -> void {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);	// disable opengl context, since using vulkan
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);		// will handle resizing manually
		this->window = glfwCreateWindow(
			this->width,
			this->height,
			this->windowName.c_str(),
			nullptr,									// windowed mode
			nullptr										// opengl context, so unused
		);
		glfwSetWindowUserPointer(this->window, this);
		glfwSetFramebufferSizeCallback(this->window, Window::frambufferResizeCallback);
	}
}
