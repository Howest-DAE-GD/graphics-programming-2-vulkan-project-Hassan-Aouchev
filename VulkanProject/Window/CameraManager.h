#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
class CameraManager {
public:
	CameraManager(glm::vec3 position = glm::vec3(0.0f, 1.f, -0.3f),
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
		float yaw = -90.f,
		float pitch = 0.0f);
	glm::mat4 GetViewMatrix() const;
	glm::vec3 GetPosition() const { return m_Position; }
	glm::vec3 GetForward() const { return m_Forward; }

	void ProcessKeyboard(int key, float deltaTime);
	void ProcessMouseMovement(float xOffset, float yOffset, bool constraintPitch = true);
	void ProcessMouseScroll(float yOffset);

	void SetPosition(const glm::vec3& postion) { m_Position = postion; }
	void SetSpeed(float speed) { m_MovementSpeed = speed; }
	void SetSensitivity(float sensitivity) { m_MouseSensitivity = sensitivity; }

private:
	void UpdateVectors();
	glm::vec3 m_Position;
	glm::vec3 m_Forward;
	glm::vec3 m_Up;
	glm::vec3 m_Right;
	glm::vec3 m_WorldUp;

	float m_Yaw;
	float m_Pitch;
	float m_MovementSpeed = 2.5f;
	float m_MouseSensitivity = 0.1f;
	float m_Fov = 45.0f;
};