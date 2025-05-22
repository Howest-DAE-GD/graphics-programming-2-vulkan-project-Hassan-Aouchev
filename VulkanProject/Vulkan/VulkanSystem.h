#pragma once

#include "../Common/ApplicationConfig.h"
#include "../Window/WindowManager.h"
#define VK_ENABLE_BETA_EXTENSIONS
#include <vulkan/vulkan.h>
#include <vector>

// Forward declarations
class Instance;
class Device;
class ResourceManager;
class SwapChain;
class PipelineManager;
class CommandManager;
class Renderer;
class CameraManager;
class Scene;

class VulkanSystem {
public:
    VulkanSystem(const ApplicationConfig& config);

    ~VulkanSystem();

    bool Initialize(WindowManager* windowManager);

    void Render();

    void WaitForDeviceIdle();

    void HandleResize(int width, int height);

    bool FramebufferResized() const { return m_FramebufferResized; }

    void ResetFramebufferResizedFlag() { m_FramebufferResized = false; }

private:

    void RecreateSwapChain();

    bool SetupDebugMessenger();

    bool CheckValidationLayerSupport();

    std::vector<const char*> GetRequiredExtensions();

    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger);

    void DestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator);

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    void Cleanup();

    ApplicationConfig m_Config;

    const std::vector<const char*> m_ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    bool m_FramebufferResized = false;

#ifdef NDEBUG
    const bool m_EnableValidationLayers = false;
#else
    const bool m_EnableValidationLayers = true;
#endif

    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;


    Instance* m_Instance = nullptr;
    Device* m_PhysicalDevice = nullptr;
    ResourceManager* m_ResourceManager = nullptr;
    PipelineManager* m_PipelineManager = nullptr;
    CommandManager* m_CommandManager = nullptr;
    Renderer* m_Renderer = nullptr;
    SwapChain* m_SwapChain = nullptr;
    Scene* m_Scene = nullptr;
    CameraManager* m_Camera = nullptr;
};