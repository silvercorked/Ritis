#pragma once

#include "Model.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <cinttypes>

namespace engine {

	struct TransformComponent {
		glm::vec3 translation{};		// position offset
		glm::vec3 scale{1.f, 1.f, 1.f};	// scale
		glm::vec3 rotation{};			// rotation (radians)

		// Matrix corresponds to translate * Ry * Rx * Rz * scale transformation
		// Rotation convention uses Tait-Bryan Angles with axis order Y, X, Z
		// R = Y X Z
		// To interpret as an extrinsic rotation: read from right to left
		// To interpret as an intrinsic rotation: read from left to right
		glm::mat4 mat4() {
			/*
				[ a b c tx
				  d e f ty
				  g h i tz
				  0 0 0 1  ]
				  // 3d affine transformation via homogeneous coordinates
			*/
			// y x z Tait-Bryan Angle (Euler Angle) https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
			const float c3 = glm::cos(rotation.z);
			const float s3 = glm::sin(rotation.z);
			const float c2 = glm::cos(rotation.x);
			const float s2 = glm::sin(rotation.x);
			const float c1 = glm::cos(rotation.y);
			const float s1 = glm::sin(rotation.y);
			return glm::mat4{
				{ // col 1
					scale.x* (c1* c3 + s1 * s2 * s3),		// a
					scale.x* (c2* s3),						// d
					scale.x* (c1* s2* s3 - c3 * s1),		// g
					0.0f									// 0
				},
				{
					scale.y * (c3 * s1 * s2 - c1 * s3),		// b
					scale.y * (c2 * c3),					// e
					scale.y * (c1 * c3 * s2 + s1 * s3),		// h
					0.0f									// 0
				},
				{
					scale.z * (c2 * s1),					// c
					scale.z * (-s2),						// f
					scale.z * (c1 * c2),					// i
					0.0f									// 0
				},
				{ translation.x, translation.y, translation.z, 1.0f} // tx, ty, tz, 1
			};
		}
	};

	struct GameObject {
		using id_t = uint32_t;

		static GameObject createGameObject() {
			static id_t currentId = 0;
			return GameObject{ currentId++ };
		}

		GameObject(const GameObject&) = delete;
		GameObject& operator=(const GameObject&) = delete;

		GameObject(GameObject&&) = default;
		GameObject& operator=(GameObject&&) = default;

		id_t getId() { return id; }

		std::shared_ptr<Model> model{};
		glm::vec3 color{};
		TransformComponent transform{};
		//RigidBodyComponent rigidBody{};
		
	private:
		id_t id;
		GameObject(id_t objId) : id{ objId } {}

	};
}
