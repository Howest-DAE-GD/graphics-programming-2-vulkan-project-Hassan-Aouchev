#pragma once

#include <GLFW/glfw3.h>
#include <functional>

class WindowManager {
public:
    WindowManager(int width, int height, const char* title);

    ~WindowManager();

    bool Initialize();

    bool ShouldClose() const;

    void PollEvents() const;

    GLFWwindow* GetWindow() const;

    int GetWidth() const { return m_Width; }

    int GetHeight() const { return m_Height; }

    bool WasResized() const { return m_FramebufferResized; }

    void ResetResizedFlag() { m_FramebufferResized = false; }

    void SetResizeCallback(std::function<void(int, int)> callback) { m_ResizeCallback = callback; }

private:

    static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

    int m_Width;
    int m_Height;
    const char* m_Title;
    bool m_Resizable;
    bool m_FramebufferResized = false;
    GLFWwindow* m_Window;
    std::function<void(int, int)> m_ResizeCallback = nullptr;
};