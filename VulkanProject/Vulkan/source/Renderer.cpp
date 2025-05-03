#include "Renderer.h"
#include "Device.h"
#include "PipelineManager.h"
#include "SwapChain.h"
#include "CommandManager.h"
#include "ResourceManager.h"
#include "Instance.h"

#include <array>
#include <chrono>
#include <GLFW/glfw3.h>
#include <algorithm>

Renderer::Renderer(Device* device, PipelineManager* pipelineManager,
                    SwapChain* swapChain, CommandManager* commandManager,
                    ResourceManager* resourceManager,Instance* instance):
	m_Device(device),
	m_PipelineManager(pipelineManager),
	m_SwapChain(swapChain),
	m_CommandManager(commandManager),
	m_ResourceManager(resourceManager),
	m_Instance(instance)
{
    glfwSetFramebufferSizeCallback(m_Instance->GetWindow(), framebufferResizeCallback);
}

Renderer::~Renderer()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(m_Device->GetDevice(), m_ImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_Device->GetDevice(), m_RenderFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_Device->GetDevice(), m_InFlightFences[i], nullptr);
    }
}

void Renderer::UpdatePushConstants(VkCommandBuffer commandBuffer)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period >(currentTime - startTime).count();
    
    int index = 0;

    for (const auto& model : m_ResourceManager->GetPushConstants())
    {
        vkCmdPushConstants(
            commandBuffer,
            m_PipelineManager->GetPipelineLayout(),
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(PushConstantData),
            &model
        );
        index++;
    }

}

void Renderer::UpdateUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period >(currentTime - startTime).count();

    // Camera position with sinusoidal movement on the Y-axis over time
    float cameraY = 50.0f;  // Oscillate around 1000 on Y-axis
    glm::vec3 cameraPosition = glm::vec3(800.f, cameraY, -30.f);  // Update Y position over time
    glm::vec3 target = glm::vec3(0.0f, 400.0f - 250.0f * sin(time), -30.0f);  // Camera looking at the origin (adjust as needed)
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);  // Y-axis as the "up" direction

    // Create the view matrix using glm::lookAt
    glm::mat4 viewMatrix = glm::lookAt(cameraPosition, target, up);

    // Prepare the uniform buffer object
    UniformBufferObject ubo{};
    ubo.view = viewMatrix;
    ubo.proj = glm::perspective(
        glm::radians(45.0f),
        m_SwapChain->GetSwapChainExtent().width / (float)m_SwapChain->GetSwapChainExtent().height,
        0.1f,   // Near plane
        5000.0f // Far plane
    );

    ubo.proj[1][1] *= -1; // Flip Y-axis for Vulkan coordinate system

    // Copy the updated uniform data to the buffer
    memcpy(m_ResourceManager->GetUniformBuffersMapped()[currentImage], &ubo, sizeof(ubo));
}
void Renderer::DrawFrame()
{
    vkWaitForFences(m_Device->GetDevice(), 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_Device->GetDevice(), m_SwapChain->GetSwapChain(), UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(m_Device->GetDevice(), 1, &m_InFlightFences[m_CurrentFrame]);
    vkResetCommandBuffer(m_CommandManager->GetCommandBuffers()[m_CurrentFrame], 0);

    UpdateUniformBuffer(m_CurrentFrame);
    RecordCommandBuffer(m_CommandManager->GetCommandBuffers()[m_CurrentFrame], imageIndex);

    if (m_Device->IsSynchronization2Supported()) {
		VkCommandBufferSubmitInfo cmdBufferSubmitInfo{};
		cmdBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		cmdBufferSubmitInfo.commandBuffer = m_CommandManager->GetCommandBuffers()[m_CurrentFrame];
		cmdBufferSubmitInfo.deviceMask = 0;

		VkSemaphoreSubmitInfo waitSemaphoreSubmitInfo{};
		waitSemaphoreSubmitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		waitSemaphoreSubmitInfo.semaphore = m_ImageAvailableSemaphores[m_CurrentFrame];
		waitSemaphoreSubmitInfo.stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		waitSemaphoreSubmitInfo.deviceIndex = 0;
		waitSemaphoreSubmitInfo.value = 0;

		VkSemaphoreSubmitInfo signalSemaphoreSubmitInfo{};
		signalSemaphoreSubmitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalSemaphoreSubmitInfo.semaphore = m_RenderFinishedSemaphores[m_CurrentFrame];
		signalSemaphoreSubmitInfo.stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		signalSemaphoreSubmitInfo.deviceIndex = 0;
		signalSemaphoreSubmitInfo.value = 0;

		VkSubmitInfo2 submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.flags = 0;
		submitInfo.waitSemaphoreInfoCount = 1;
		submitInfo.pWaitSemaphoreInfos = &waitSemaphoreSubmitInfo;
		submitInfo.commandBufferInfoCount = 1;
		submitInfo.pCommandBufferInfos = &cmdBufferSubmitInfo;
		submitInfo.signalSemaphoreInfoCount = 1;
		submitInfo.pSignalSemaphoreInfos = &signalSemaphoreSubmitInfo;

        if (vkQueueSubmit2(m_Device->GetGraphicsQueue(), 1, &submitInfo,
            m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
    }
    else {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_CommandManager->GetCommandBuffers()[m_CurrentFrame];
        VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw  command buffer!");
        }
    }
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrame];
    VkSwapchainKHR swapChains[] = { m_SwapChain->GetSwapChain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(m_Device->GetPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized) {
        m_FramebufferResized = false;
        RecreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::CreateSyncObjects()
{
    m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        if (vkCreateSemaphore(m_Device->GetDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_Device->GetDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_Device->GetDevice(), &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create semaphores");
        }
    }
}

void Renderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_PipelineManager->GetRenderPass();
    renderPassInfo.framebuffer = m_SwapChain->GetSwapChainFramebuffers()[imageIndex];
    renderPassInfo.renderArea.offset = { 0,0 };
    renderPassInfo.renderArea.extent = m_SwapChain->GetSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewPort{};
    viewPort.x = 0.0f;
    viewPort.y = 0.0f;
    viewPort.width = static_cast<float>(m_SwapChain->GetSwapChainExtent().width);
    viewPort.height = static_cast<float>(m_SwapChain->GetSwapChainExtent().height);
    viewPort.minDepth = 0.0f;
    viewPort.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewPort);

    VkRect2D scissor{};
    scissor.offset = { 0,0 };
    scissor.extent = m_SwapChain->GetSwapChainExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineManager->GetGraphicsPipeline());

    const auto& meshes = m_ResourceManager->GetMeshes(); // Get all meshes
    const auto& pushConstants = m_ResourceManager->GetPushConstants(); // Get all push constants

    // Bind vertex and index buffers once if they're shared
    VkBuffer vertexBuffers[] = { m_ResourceManager->GetVertexBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_ResourceManager->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    for (size_t i = 0; i < meshes.size(); ++i) {
        const auto& mesh = meshes[i];

        // Make sure you have a push constant for each mesh
        if (i < pushConstants.size()) {
            // Update push constants for this specific mesh
            vkCmdPushConstants(
                commandBuffer,
                m_PipelineManager->GetPipelineLayout(),
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(PushConstantData),
                &pushConstants[i]
            );
        }

        // Bind descriptor set (if needed per mesh)
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_PipelineManager->GetPipelineLayout(), 0, 1,
            &m_ResourceManager->GetDescriptorSets()[m_CurrentFrame], 0, nullptr);

        // Draw the current mesh
        vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, mesh.indexOffset, 0, 0);

    }
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void Renderer::RecreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_Instance->GetWindow(), &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_Instance->GetWindow(), &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(m_Device->GetDevice());

    m_ResourceManager->CleanDepth();
    m_SwapChain->CleanupSwapchain();

    m_SwapChain->CreateSwapChain();
    m_SwapChain->CreateImageViews();
    m_ResourceManager->CreateDepthResources(m_SwapChain);
    m_SwapChain->CreateFrameBuffers(m_PipelineManager,m_ResourceManager);
}
