#pragma once

#include "Camera.hpp"
#include "GameObject.hpp"

#include <vulkan/vulkan.h>

namespace engine {
	struct FrameInfo {
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		Camera& camera;
		VkDescriptorSet globalDescriptorSet;
		GameObject::Map& gameObjects;
	};
}
