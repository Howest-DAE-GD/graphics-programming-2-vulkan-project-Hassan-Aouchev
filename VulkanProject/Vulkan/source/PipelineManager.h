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
	VkPipeline GetGraphicsPipeline() const { return m_ForwardPipeline; }
	VkPipelineLayout GetPipelineLayout() const { return m_ForwardPipelineLayout; }

	void CreateDepthPrepassPipeline();
	VkPipeline GetDepthPrepassPipeline() const { return m_DepthPrepassPipeline; }
	VkPipelineLayout GetDepthPrepassPipelineLayout() const { return m_DepthPrepassPipelineLayout; }

	void CreateGBufferPipeline();
	VkPipeline GetGBufferPipeline() const { return m_GBufferPipeline; }
	VkPipelineLayout GetGBufferPipelineLayout() const { return m_GBufferPipelineLayout; }

	void CreateLightingPipeline();
	VkDescriptorSetLayout& GetLightingDescriptorSetLayout() { return m_LightingDescriptorSetLayout; }
	VkPipeline GetLightingPipeline() const { return m_LightingPipeline; }
	VkPipelineLayout GetLightingPipelineLayout() const { return m_LightingPipelineLayout; }


private:
	void CreateDescriptorSetLayout();
	void CreateLightingDescriptorSetLayout();
	void CreateForwardPipeline();
	void SavePipelineCache();

	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	VkPipeline m_ForwardPipeline;
	VkPipelineLayout m_ForwardPipelineLayout;
	VkPipelineCache m_PipelineCache;
	VkDescriptorSetLayout m_DescriptorSetLayout;
	VkPipeline m_GBufferPipeline;
	VkPipelineLayout m_GBufferPipelineLayout;

	VkDescriptorSetLayout m_LightingDescriptorSetLayout;
	VkPipeline m_LightingPipeline;
	VkPipelineLayout m_LightingPipelineLayout;

	VkPipeline m_DepthPrepassPipeline;
	VkPipelineLayout m_DepthPrepassPipelineLayout;

	std::vector<VkDescriptorSetLayoutBinding> m_Bindings{};
	std::vector<VkDescriptorBindingFlags> m_BindingFlags{};

	ResourceManager* m_ResourceManager;
	SwapChain* m_SwapChain;
	Device* m_Device;
};