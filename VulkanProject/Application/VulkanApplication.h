#pragma once

#include "../Common/ApplicationConfig.h"
#include "../Window/WindowManager.h"
#include "../Vulkan/VulkanSystem.h"

class VulkanApplication {
public:
    VulkanApplication(const ApplicationConfig& config = ApplicationConfig());

    void Run();

private:
    bool Initialize();

    void MainLoop();

    ApplicationConfig m_Config;
    WindowManager m_WindowManager;
    VulkanSystem m_VulkanSystem;
};