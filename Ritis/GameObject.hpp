#pragma once

#include "Model.hpp"

#include <memory>
#include <cinttypes>

namespace engine {

	struct Transform2dComponent {
		glm::vec2 translation{};	// position offset
		glm::vec2 scale{1.f, 1.f};	// scale
		float rotation;				// rotation (radians)
		glm::mat2 mat2() {
			const float c = glm::cos(this->rotation);
			const float s = glm::sin(this->rotation);
			glm::mat2 rotationMat{			// [ cos(t)  -sin(t) ] // rotation matrix
				{ c, s },					// [ sin(t)   cos(t) ]
				{ -s, c }
			};
			glm::mat2 scaleMat{				// [ Sx   0  ] // scale transformation matrix
				{this->scale.x, 0},			// [ 0    Sy ]
				{0, this->scale.y}
			};
			return rotationMat * scaleMat; // linear combination
			// order of application of transformations is right to left.
			// in shader, this allows multiplying position vectors from left side (ie. transformMat * positionVec).
			// column-major order
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
		Transform2dComponent transform2d{};
		
	private:
		id_t id;
		GameObject(id_t objId) : id{ objId } {}

	};
}
