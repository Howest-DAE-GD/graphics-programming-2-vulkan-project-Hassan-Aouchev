#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class Device;
class Instance;
class ResourceManager;
class GLFWwindow;
class PipelineManager;
class SwapChain
{
public:
	SwapChain(Device* device, Instance* instance,ResourceManager* resourceManager);

	VkSwapchainKHR GetSwapChain() const { return m_SwapChain; }
	VkFormat GetSwapChainImageFormat() const { return m_SwapChainImageFormat; }
	VkExtent2D GetSwapChainExtent() const { return m_SwapChainExtent; }
	std::vector<VkFramebuffer> GetSwapChainFramebuffers() const { return m_SwapChainFramebuffers; }

	void CleanupSwapchain();
	void CreateSwapChain();
	void CreateImageViews();
	void CreateFrameBuffers(PipelineManager* pipelineManager, ResourceManager* resourceManager);
private:

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	VkSwapchainKHR m_SwapChain;
	std::vector<VkImage> m_SwapChainImages;
	VkFormat m_SwapChainImageFormat;
	VkExtent2D m_SwapChainExtent;
	std::vector<VkImageView> m_SwapChainImageViews;
	std::vector<VkFramebuffer> m_SwapChainFramebuffers;

	Device* m_Device;
	ResourceManager* m_ResourceManager;
	Instance* m_Instance;
};