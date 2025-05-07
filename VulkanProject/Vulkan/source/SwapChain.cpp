#pragma once
#include "SwapChain.h"
#include <stdexcept>
#include <algorithm>
#include <array>

#include "Device.h"
#include "Instance.h"
#include "ResourceManager.h"
#include "PipelineManager.h"

SwapChain::SwapChain(Device* device, Instance* instance, ResourceManager* resourceManager)
    : m_Device(device), m_Instance(instance), m_ResourceManager(resourceManager)
{
	CreateSwapChain();
	CreateImageViews();
}

SwapChain::~SwapChain()
{
    for (auto image : m_SwapChainImages) {
        delete image;
    }
    m_SwapChainImages.clear();
}

void SwapChain::CreateSwapChain()
{
    SwapChainSupportDetails swapChainSupport = m_Device->QuerySwapChainSupport(m_Device->GetPhysicalDevice());

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_Instance->GetSurface();

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = m_Device->FindQueueFamilies(m_Device->GetPhysicalDevice());
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_Device->GetDevice(), &createInfo, nullptr, &m_SwapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(m_Device->GetDevice(), m_SwapChain, &imageCount, nullptr);

    m_SwapChainImages.clear();
    m_SwapChainImages.resize(imageCount);

	std::vector<VkImage> tempSwapChainImages(imageCount);

    vkGetSwapchainImagesKHR(m_Device->GetDevice(), m_SwapChain, &imageCount, tempSwapChainImages.data());
	
    for (int i = 0; i < imageCount; i++)
    {
        m_SwapChainImages[i] = new Image();
        m_SwapChainImages[i]->image = tempSwapChainImages[i];
        m_SwapChainImages[i]->currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        m_SwapChainImages[i]->format = surfaceFormat.format;
    }

    m_SwapChainImageFormat = surfaceFormat.format;
    m_SwapChainExtent = extent;
}

void SwapChain::CreateImageViews()
{
    m_SwapChainImageViews.resize(m_SwapChainImages.size());

    for (size_t i = 0; i < m_SwapChainImages.size(); i++) {
        m_SwapChainImageViews[i] = m_ResourceManager->CreateImageView(m_SwapChainImages[i]->image, m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void SwapChain::CreateFrameBuffers(PipelineManager* pipelineManager,ResourceManager* resourceManager)
{
    m_SwapChainFramebuffers.resize(m_SwapChainImageViews.size());

    for (size_t i = 0; i < m_SwapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            m_SwapChainImageViews[i],
            resourceManager->GetDepthImageView()
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = pipelineManager->GetRenderPass();
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_SwapChainExtent.width;
        framebufferInfo.height = m_SwapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_Device->GetDevice(), &framebufferInfo, nullptr, &m_SwapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer");
        }
    }
}




void SwapChain::CleanupSwapchain()
{

    for (size_t i = 0; i < m_SwapChainFramebuffers.size(); i++) {
        vkDestroyFramebuffer(m_Device->GetDevice(), m_SwapChainFramebuffers[i], nullptr);
    }
    for (size_t i = 0; i < m_SwapChainImageViews.size(); i++) {
        vkDestroyImageView(m_Device->GetDevice(), m_SwapChainImageViews[i], nullptr);
    }
    vkDestroySwapchainKHR(m_Device->GetDevice(), m_SwapChain, nullptr);

    m_SwapChainFramebuffers.clear();
    m_SwapChain = nullptr;


}

VkSurfaceFormatKHR SwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR SwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(m_Instance->GetWindow(), &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}
