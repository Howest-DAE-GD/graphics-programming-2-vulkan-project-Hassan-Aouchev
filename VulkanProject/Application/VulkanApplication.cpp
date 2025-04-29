#include "VulkanApplication.h"
#include <iostream>
#include <stdexcept>

VulkanApplication::VulkanApplication(const ApplicationConfig& config)
    : m_Config(config),
    m_WindowManager(config.width, config.height, config.applicationName.c_str()),
    m_VulkanSystem(config) {
}

void VulkanApplication::Run() {
    if (!Initialize()) {
        throw std::runtime_error("Failed to initialize application");
    }

    MainLoop();
}

bool VulkanApplication::Initialize() {
    if (!m_WindowManager.Initialize()) {
        std::cerr << "Failed to initialize window" << std::endl;
        return false;
    }

    if (!m_VulkanSystem.Initialize(&m_WindowManager)) {
        std::cerr << "Failed to initialize Vulkan system" << std::endl;
        return false;
    }

    return true;
}

void VulkanApplication::MainLoop() {
    while (!m_WindowManager.ShouldClose()) {
        m_WindowManager.PollEvents();
        m_VulkanSystem.Render();
    }

    m_VulkanSystem.WaitForDeviceIdle();
}