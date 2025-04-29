#pragma once
#include <vector>
#include <vulkan/vulkan.h>

class Device;
class CommandManager {
public:
	CommandManager(Device* device);

	void CreateCommandPool();
	void CreateCommandBuffers();

	void CleanCommandPool();
	void CleanCommandBuffers();

    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
    VkCommandBuffer BeginSingleTimeCommands();

	std::vector<VkCommandBuffer>& GetCommandBuffers() { return m_CommandBuffers; }
private:


    VkCommandPool m_CommandPool;
    std::vector<VkCommandBuffer> m_CommandBuffers;

	Device* m_Device;
};