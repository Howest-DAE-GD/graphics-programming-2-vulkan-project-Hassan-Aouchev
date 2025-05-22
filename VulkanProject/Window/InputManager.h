#pragma once
#include <GLFW/glfw3.h>
#include <unordered_set>

class CameraManager;

class InputManager {
public:
	static InputManager& Instance();

	void Initialize(GLFWwindow* window);
	void Update(float deltaTime);

	bool IsKeyPressed(int key)const;
	bool IsKeyJustPressed(int key)const;

	void SetCamera(CameraManager* camera) { m_Camera = camera; }

private:
	InputManager() = default;

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void MouseCallback(GLFWwindow* window, double xPos, double yPos);
	static void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

	GLFWwindow* m_Window = nullptr;
	CameraManager* m_Camera;

	std::unordered_set<int> m_JustPressedKeys;

	double m_LastMouseX = 0.0;
	double m_LastMouseY = 0.0;
	bool m_FirstMouse = true;
};