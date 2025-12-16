#pragma once

#include <cmath>
#include <glm/glm.hpp>
#include "config.hpp"

struct Mouse {
	glm::vec2 current;
	glm::vec2 previous;
	bool dragging;
};

struct Camera {
	glm::vec2 position;
	glm::vec2 velocity;
	float scale = 1.0f;
	float deltaScale;
	glm::vec2 scalePivot;

	glm::vec2 world(const glm::vec2& v) const {
		return v / scale;
	}

	void update(const Config& cfg, 
		float dt, 
		const Mouse& mouse, 
		const glm::vec2& windowSize) {
		if (std::abs(deltaScale) > 0.5f) {
			glm::vec2 p0 = (scalePivot - (windowSize * 0.5f)) / scale;
			scale = std::max(scale + deltaScale * dt, cfg.minScale);
			glm::vec2 p1 = (scalePivot - (windowSize * 0.5f)) / scale;
			position += p0 - p1;

			deltaScale -= deltaScale * dt * cfg.scaleFriction; 
		}
		if (!mouse.dragging && (glm::length(velocity) > cfg.velocityThreshold)) {
			position += velocity * dt;
			velocity -= velocity * dt * cfg.scaleFriction;
		}
	}
};

