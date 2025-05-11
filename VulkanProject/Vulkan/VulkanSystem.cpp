#include "VulkanSystem.h"
#include "source/Instance.h"
#include "source/Device.h"
#include "source/ResourceManager.h"
#include "source/SwapChain.h"
#include "source/PipelineManager.h"
#include "source/CommandManager.h"
#include "source/Renderer.h"
#include "source/Scene.h"
#include <iostream>
#include <stdexcept>
#include <cstring>

VulkanSystem::VulkanSystem(const ApplicationConfig& config)
    : m_Config(config) {
}

VulkanSystem::~VulkanSystem() {
    Cleanup();
}

bool VulkanSystem::Initialize(WindowManager* windowManager) {
    try {
        if (m_EnableValidationLayers && !CheckValidationLayerSupport()) {
            throw std::runtime_error("Validation layers requested, but not available!");
        }

        m_Instance = new Instance(windowManager->GetWindow());

        if (m_EnableValidationLayers) {
            if (!SetupDebugMessenger()) {
                std::cerr << "Warning: Failed to set up debug messenger" << std::endl;
            }
        }

        m_PhysicalDevice = new Device(m_Instance->GetInstance(), m_Instance->GetSurface());
        m_ResourceManager = new ResourceManager(m_PhysicalDevice);
        m_SwapChain = new SwapChain(m_PhysicalDevice, m_Instance, m_ResourceManager);
        m_Scene = new Scene(m_ResourceManager);
        m_Scene->LoadScene(m_Config.scenePath);
        m_PipelineManager = new PipelineManager(m_PhysicalDevice, m_ResourceManager, m_SwapChain);
        m_CommandManager = new CommandManager(m_PhysicalDevice);


        m_Renderer = new Renderer(m_PhysicalDevice, m_PipelineManager, m_SwapChain,
            m_CommandManager, m_ResourceManager, m_Instance);

        m_ResourceManager->SetCommandManager(m_CommandManager);
        m_CommandManager->CreateCommandPool();
        m_ResourceManager->Create(m_SwapChain, m_PipelineManager);
        m_CommandManager->CreateCommandBuffers();
        m_Renderer->CreateSyncObjects();

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Initialization error: " << e.what() << std::endl;
        Cleanup();
        return false;
    }
}

bool VulkanSystem::CheckValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : m_ValidationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

std::vector<const char*> VulkanSystem::GetRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (m_EnableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void VulkanSystem::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
    createInfo.pUserData = nullptr;
}

bool VulkanSystem::SetupDebugMessenger() {
    if (!m_EnableValidationLayers) return true;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    PopulateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(m_Instance->GetInstance(), &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS) {
        return false;
    }

    return true;
}

VkResult VulkanSystem::CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void VulkanSystem::DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator) {

    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanSystem::DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        std::cerr << "Validation ERROR: " << pCallbackData->pMessage << std::endl;
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "Validation WARNING: " << pCallbackData->pMessage << std::endl;
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        std::cout << "Validation INFO: " << pCallbackData->pMessage << std::endl;
    }
    else {
        std::cout << "Validation VERBOSE: " << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
}

void VulkanSystem::Render() {
    m_Renderer->DrawFrame();
}

void VulkanSystem::WaitForDeviceIdle() {
    vkDeviceWaitIdle(m_PhysicalDevice->GetDevice());
}

void VulkanSystem::HandleResize(int width, int height) {
    m_FramebufferResized = true;
    std::cout << "Window resized to: " << width << "x" << height << std::endl;
}

void VulkanSystem::RecreateSwapChain() {
    m_Renderer->RecreateSwapChain();

    m_FramebufferResized = false;
}

void VulkanSystem::Cleanup() {
    if (m_ResourceManager) m_ResourceManager->CleanDepth();
    if (m_SwapChain) m_SwapChain->CleanupSwapchain();

    if (m_DebugMessenger != VK_NULL_HANDLE && m_Instance) {
        DestroyDebugUtilsMessengerEXT(m_Instance->GetInstance(), m_DebugMessenger, nullptr);
        m_DebugMessenger = VK_NULL_HANDLE;
    }

    delete m_SwapChain;
    delete m_ResourceManager;
    delete m_PipelineManager;

    if (m_CommandManager) m_CommandManager->CleanCommandPool();
    delete m_CommandManager;
    delete m_Renderer;
    delete m_Scene;
    delete m_PhysicalDevice;
    delete m_Instance;

    m_SwapChain = nullptr;
    m_ResourceManager = nullptr;
    m_PipelineManager = nullptr;
    m_CommandManager = nullptr;
    m_Renderer = nullptr;
    m_Scene = nullptr;
    m_PhysicalDevice = nullptr;
    m_Instance = nullptr;
}