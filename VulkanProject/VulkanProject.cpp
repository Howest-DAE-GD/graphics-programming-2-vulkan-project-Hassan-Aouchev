#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <string>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>

#include <fstream>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <array>

#include "source/Device.h"
#include "source/Instance.h"
#include "source/ResourceManager.h"
#include "source/SwapChain.h"
#include "source/CommandManager.h"
#include "source/PipelineManager.h"
#include "source/Renderer.h"


VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}


const int WIDTH = 800;
const int HEIGHT = 600;

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;

    Instance* m_Instance;
	Device* m_PhysicalDevice;
	ResourceManager* m_ResourceManager;
	PipelineManager* m_PipelineManager;
	CommandManager* m_CommandManager;
	Renderer* m_Renderer;
	SwapChain* m_SwapChain;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
    }

    void initVulkan() {
		m_Instance = new Instance(window);
		m_PhysicalDevice = new Device(m_Instance->GetInstance(), m_Instance->GetSurface());
        m_ResourceManager = new ResourceManager(m_PhysicalDevice);
		m_SwapChain = new SwapChain(m_PhysicalDevice, m_Instance, m_ResourceManager);
		m_PipelineManager = new PipelineManager(m_PhysicalDevice, m_ResourceManager,m_SwapChain);
		m_CommandManager = new CommandManager(m_PhysicalDevice);

		m_Renderer = new Renderer(m_PhysicalDevice, m_PipelineManager, m_SwapChain, m_CommandManager, m_ResourceManager, m_Instance);

        m_ResourceManager->SetCommandManager(m_CommandManager);

        m_CommandManager->CreateCommandPool();
        m_ResourceManager->Create(m_SwapChain,m_PipelineManager);
		m_SwapChain->CreateFrameBuffers(m_PipelineManager,m_ResourceManager);
        m_CommandManager->CreateCommandBuffers();
		m_Renderer->CreateSyncObjects();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            m_Renderer->DrawFrame();
        }
        vkDeviceWaitIdle(m_PhysicalDevice->GetDevice());
    }
    void cleanup() {
        m_ResourceManager->CleanDepth();
        m_SwapChain->CleanupSwapchain();

        delete m_ResourceManager;

        delete m_PipelineManager;

        m_CommandManager->CleanCommandPool();

        delete m_Renderer;
        m_Renderer = nullptr;

        delete m_PhysicalDevice;
		m_PhysicalDevice = nullptr;

        delete m_Instance;
        m_Instance = nullptr;

        glfwDestroyWindow(window);

        glfwTerminate();

    }
};


int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}