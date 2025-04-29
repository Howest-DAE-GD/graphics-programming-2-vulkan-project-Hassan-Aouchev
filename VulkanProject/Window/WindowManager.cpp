#include "WindowManager.h"
#include <iostream>

WindowManager::WindowManager(int width, int height, const char* title)
    : m_Width(width), m_Height(height), m_Title(title), m_Window(nullptr) {
}

WindowManager::~WindowManager() {
    if (m_Window) {
        glfwDestroyWindow(m_Window);
    }
    glfwTerminate();
}

bool WindowManager::Initialize() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_Window = glfwCreateWindow(m_Width, m_Height, m_Title, nullptr, nullptr);
    if (!m_Window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwSetWindowUserPointer(m_Window, this);

    glfwSetFramebufferSizeCallback(m_Window, FramebufferResizeCallback);

    return true;
}

bool WindowManager::ShouldClose() const {
    return glfwWindowShouldClose(m_Window);
}

void WindowManager::PollEvents() const {
    glfwPollEvents();
}

GLFWwindow* WindowManager::GetWindow() const {
    return m_Window;
}
void WindowManager::FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto windowManager = reinterpret_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    windowManager->m_Width = width;
    windowManager->m_Height = height;
    windowManager->m_FramebufferResized = true;

    if (windowManager->m_ResizeCallback) {
        windowManager->m_ResizeCallback(width, height);
    }
}