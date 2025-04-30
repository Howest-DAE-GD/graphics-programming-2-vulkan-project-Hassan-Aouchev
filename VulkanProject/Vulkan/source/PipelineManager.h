#pragma once
#include <vulkan/vulkan.h>
#include <fstream>
#include <vector>

static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);

	return buffer;
}
class ResourceManager;
class SwapChain;
class Device;
class PipelineManager
{
public:
	PipelineManager(Device* device,ResourceManager* resourceManager, SwapChain* swapChain);
	~PipelineManager();

	VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }
	VkRenderPass GetRenderPass() const { return m_RenderPass; }
	VkPipeline GetGraphicsPipeline() const { return m_GraphicsPipeline; }
	VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }


private:
	void CreateRenderPass();
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	void SavePipelineCache();

	VkShaderModule CreateShaderModule(const std::vector<char>& code);


	VkRenderPass m_RenderPass;
	VkPipelineLayout m_PipelineLayout;
	VkPipelineCache m_PipelineCache;
	VkPipeline m_GraphicsPipeline;
	VkDescriptorSetLayout m_DescriptorSetLayout;

	ResourceManager* m_ResourceManager;
	SwapChain* m_SwapChain;
	Device* m_Device;
};