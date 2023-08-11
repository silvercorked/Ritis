#pragma once

#define GLFW_INCLUDE__VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace engine {
	class Window {
		GLFWwindow* window;
		const int width;
		const int height;
		std::string windowName;

		auto initWindow() -> void;

	public:
		Window(int w, int h, std::string name);
		~Window();

		Window(const Window&) = delete;					// copy and assignment deleted
		Window& operator=(const Window&) = delete;		// for RAII

		inline auto shouldClose() -> bool;
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
	auto Window::shouldClose() -> bool {
		return glfwWindowShouldClose(this->window);
	}

	auto Window::initWindow() -> void {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);	// disable opengl context, since using vulkan
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);		// will handle resizing manually
		window = glfwCreateWindow(
			width,
			height,
			windowName.c_str(),
			nullptr,									// windowed mode
			nullptr										// opengl context, so unused
		);
	}
}
