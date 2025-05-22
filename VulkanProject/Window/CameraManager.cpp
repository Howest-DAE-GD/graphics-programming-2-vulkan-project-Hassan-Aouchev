#include "CameraManager.h"

CameraManager::CameraManager(glm::vec3 position, glm::vec3 up, float yaw, float pitch):
	m_Position{position},
	m_WorldUp{up},
	m_Yaw{yaw},
	m_Pitch{pitch}
{
	UpdateVectors();
}

glm::mat4 CameraManager::GetViewMatrix()const {
	return glm::lookAt(m_Position, m_Position+ m_Forward, m_Up);
}

void CameraManager::ProcessKeyboard(int key, float deltaTime)
{
	float velocity = deltaTime * m_MovementSpeed;

    if (key == GLFW_KEY_W) {
        m_Position += m_Forward * velocity;
    }
    if (key == GLFW_KEY_S) {
        m_Position -= m_Forward * velocity;
    }
    if (key == GLFW_KEY_A) {
        m_Position -= m_Right * velocity;
    }
    if (key == GLFW_KEY_D) {
        m_Position += m_Right * velocity;
    }
    if (key == GLFW_KEY_SPACE) {
        m_Position += m_WorldUp * velocity;
    }
    if (key == GLFW_KEY_LEFT_SHIFT) {
        m_Position -= m_WorldUp * velocity;
    }
}

void CameraManager::ProcessMouseMovement(float xOffset, float yOffset, bool constraintPitch)
{
    xOffset *= m_MouseSensitivity;
    yOffset *= m_MouseSensitivity;

    m_Yaw += xOffset;
    m_Pitch += yOffset;

    if (constraintPitch) {
        if (m_Pitch > 89.0f) m_Pitch = 89.0f;
        if (m_Pitch < -89.0f) m_Pitch = -89.0f;
    }

    UpdateVectors();
}

void CameraManager::ProcessMouseScroll(float yOffset)
{
    m_Fov -= yOffset;
    if (m_Fov < 1.0f) m_Fov = 1.0f;
    if (m_Fov > 45.0f) m_Fov = 45.0f;
}

void CameraManager::UpdateVectors()
{
    glm::vec3 forward;
    forward.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    forward.y = sin(glm::radians(m_Pitch));
    forward.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    m_Forward = glm::normalize(forward);

    m_Right = glm::normalize(glm::cross(m_Forward, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, m_Forward));
}


