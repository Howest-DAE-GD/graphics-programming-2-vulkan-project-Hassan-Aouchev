#include "InputManager.h"
#include "CameraManager.h"

InputManager& InputManager::Instance()
{
	static InputManager instance;
	return instance;
}

void InputManager::Initialize(GLFWwindow* window)
{
	m_Window = window;
	glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetScrollCallback(window, ScrollCallback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void InputManager::Update(float deltaTime)
{
	if (m_Camera) {
		if (IsKeyPressed(GLFW_KEY_W)) m_Camera->ProcessKeyboard(GLFW_KEY_W, deltaTime);
		if (IsKeyPressed(GLFW_KEY_S)) m_Camera->ProcessKeyboard(GLFW_KEY_S, deltaTime);
		if (IsKeyPressed(GLFW_KEY_A)) m_Camera->ProcessKeyboard(GLFW_KEY_A, deltaTime);
		if (IsKeyPressed(GLFW_KEY_D)) m_Camera->ProcessKeyboard(GLFW_KEY_D, deltaTime);
		if (IsKeyPressed(GLFW_KEY_SPACE)) m_Camera->ProcessKeyboard(GLFW_KEY_SPACE, deltaTime);
		if (IsKeyPressed(GLFW_KEY_LEFT_SHIFT)) m_Camera->ProcessKeyboard(GLFW_KEY_LEFT_SHIFT, deltaTime);
	}
}

bool InputManager::IsKeyPressed(int key) const
{
	return glfwGetKey(m_Window, key) == GLFW_PRESS;
}

bool InputManager::IsKeyJustPressed(int key) const
{
	return m_JustPressedKeys.find(key) != m_JustPressedKeys.end();
}

void InputManager::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	InputManager* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window));

	if (action == GLFW_PRESS) {
		input->m_JustPressedKeys.insert(key);

		// Toggle cursor mode with ESC
		if (key == GLFW_KEY_ESCAPE) {
			static bool cursorEnabled = false;
			cursorEnabled = !cursorEnabled;
			glfwSetInputMode(window, GLFW_CURSOR, cursorEnabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
		}
	}
}

void InputManager::MouseCallback(GLFWwindow* window, double xPos, double yPos)
{
	InputManager* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window));

	if (input->m_FirstMouse) {
		input->m_LastMouseX = xPos;
		input->m_LastMouseY = yPos;
		input->m_FirstMouse = false;
	}

	double deltaX = xPos - input->m_LastMouseX;
	double deltaY = input->m_LastMouseY - yPos;
	input->m_LastMouseX = xPos;
	input->m_LastMouseY = yPos;

	// Only process mouse movement if cursor is disabled
	if (input->m_Camera && glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
		input->m_Camera->ProcessMouseMovement(deltaX, deltaY);
	}
}

void InputManager::ScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
	InputManager* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window));

	if (input->m_Camera) {
		input->m_Camera->ProcessMouseScroll(yOffset);
	}
}
