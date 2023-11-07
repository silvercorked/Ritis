#pragma once

#define GLM_FORCE_RADIANS					// functions expect radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// Depth buffer values will range from 0 to 1, not -1 to 1
#include <glm/glm.hpp>

#include <cassert>
#include <limits>

namespace engine {
	class Camera {
		glm::mat4 projectionMatrix{1.0f};

	public:
		auto setOrthographicProjection(float left, float right, float top, float bottom, float near, float far) -> void;
		auto setPerspectiveProjection(float fovy, float aspect, float near, float far) -> void;

		const glm::mat4& getProjection() const { return this->projectionMatrix; }
	};

	auto Camera::setOrthographicProjection(float left, float right, float top, float bottom, float near, float far) -> void {
		this->projectionMatrix = glm::mat4{ 1.0f };
		this->projectionMatrix[0][0] = 2.0f / (right - left);
		this->projectionMatrix[1][1] = 2.0f / (bottom - top);
		this->projectionMatrix[2][2] = 1.0f / (far - near);
		this->projectionMatrix[3][0] = -(right + left) / (right - left);
		this->projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
		this->projectionMatrix[3][2] = -near / (far - near);
	}
	auto Camera::setPerspectiveProjection(float fovy, float aspect, float near, float far) -> void {
		assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
		const float tanHalfFovY = tan(fovy / 2.0f);
		this->projectionMatrix = glm::mat4{ 0.0f };
		this->projectionMatrix[0][0] = 1.0f / (aspect * tanHalfFovY);
		this->projectionMatrix[1][1] = 1.0f / (tanHalfFovY);
		this->projectionMatrix[2][2] = far / (far - near);
		this->projectionMatrix[2][3] = 1.0f;
		this->projectionMatrix[3][2] = -(far * near) / (far - near);
	}
}


