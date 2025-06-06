#pragma once
#include <vulkan/vulkan.h>

#include <optional>
#include <vector>

const int MAX_FRAMES_IN_FLIGHT = 2;

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool IsComplete() const{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class Device
{
private:
	void PickPhysicalDevice(VkInstance instance);
	bool IsDeviceSuitable(VkPhysicalDevice device);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

	void CreateLogicalDevice();

	bool m_EnableValidationLayers = false;

	const std::vector<const char*> m_ValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> m_DeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
	};

	VkDevice m_Device;
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkSurfaceKHR m_Surface;

	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;
public:

	bool IsSynchronization2Supported() const { return m_Synchronization2Supported; }

	Device(VkInstance instance, VkSurfaceKHR surface,bool enableValidationLayer);
	~Device() {
		vkDestroyDevice(m_Device, nullptr);
	}

	bool m_Synchronization2Supported = false;

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
	VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
	VkDevice GetDevice() const { return m_Device; }
	VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
	VkQueue GetPresentQueue() const { return m_PresentQueue; }
};