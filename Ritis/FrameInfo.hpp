#pragma once

#include "Camera.hpp"
#include "GameObject.hpp"

#include <vulkan/vulkan.h>

namespace engine {

#define MAX_LIGHTS 10

	struct PointLight {
		glm::vec4 position{}; // ignore w
		glm::vec4 color{}; // // w is light intensity. could pack into vec3 { r * i, g * i, b * i }, but then values need to be able to be > 1
	};
	struct GlobalUniformBufferObject { // automatic alignment (std140 qualified uniform block (https://www.oreilly.com/library/view/opengl-programming-guide/9780132748445/app09lev1sec2.html)
		alignas(64) glm::mat4 projection{ 1.0f };
		alignas(64) glm::mat4 view{ 1.0f };
		alignas(64) glm::mat4 inverseView{ 1.0f }; // last column is camera position. also can be used to transform from camera to world (maybe for UI?)
		alignas(16) glm::vec4 ambientLightColor{ 1.0f, 1.0f, 1.0f, 0.02f };
		PointLight pointLights[MAX_LIGHTS]; // using alignas(32) causes alignment issue (numLights is not set right in frag shader causing only ambient color)
		int numLights;
	}; // using alignas to both be explicit and confirm my understanding

	struct FrameInfo {
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		Camera& camera;
		VkDescriptorSet globalDescriptorSet;
		GameObject::Map& gameObjects;
	};
}
