#pragma once
#include <vulkan/vulkan.h>

class SwapChain
{
private:
	VkSwapchainKHR m_SwapChain;
	std::vector<VkImage> swapChainImages;

public:
};