#pragma once
#include <vulkan/vulkan.h>
#include <fstream>
#include <vector>

struct TonemappingPushConstants {
	float averageLuminance;
	int exposureMode;
};

static std::vector<uint32_t> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("failed to open file: " + filename);
	}

	size_t fileSize = (size_t)file.tellg();
	// Ensure the size is a multiple of 4 (uint32_t size)
	if (fileSize % 4 != 0) {
		throw std::runtime_error("SPIR-V file size is not a multiple of 4: " + filename);
	}

	// Create a vector of uint32_t (which is what VkShaderModule expects)
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

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

	VkDescriptorSetLayout GetUniversalDescriptorSetLayout() const { return m_UniversalDescriptorSetLayout; }
	VkDescriptorSetLayout GetGBufferDescriptorSetLayout() const { return m_GBufferDescriptorSetLayout; }
	VkDescriptorSetLayout GetDepthPrepassDescriptorSetLayout() const { return m_DepthPrepassDescriptorSetLayout; }

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

	void CreateToneMappingPipeline();
	VkDescriptorSetLayout& GetToneMappingDescriptorSetLayout() { return m_ToneMappingDescriptorSetLayout; }
	VkPipeline GetToneMappingPipeline() const { return m_ToneMappingPipeline; }
	VkPipelineLayout GetToneMappingPipelineLayout() const { return m_ToneMappingPipelineLayout; }

	VkShaderModule CreateShaderModule(const std::vector<uint32_t>& code);
private:
	//(set 0 in both pipelines)
	void CreateUniversalDescriptorSetLayout();
	//(set 1 in main pipeline)  
	void CreateGBufferDescriptorSetLayout();
	//(set 1 in depth pipeline)
	void CreateDepthPrepassDescriptorSetLayout();

	void CreateToneMappingDescriptorSetLayout();

	void CreateLightingDescriptorSetLayout();
	void SavePipelineCache();


	VkPipelineCache m_PipelineCache;
	VkDescriptorSetLayout m_UniversalDescriptorSetLayout;
	VkDescriptorSetLayout m_GBufferDescriptorSetLayout;
	VkDescriptorSetLayout m_DepthPrepassDescriptorSetLayout;
	VkPipeline m_GBufferPipeline;
	VkPipelineLayout m_GBufferPipelineLayout;

	VkDescriptorSetLayout m_LightingDescriptorSetLayout;
	VkPipeline m_LightingPipeline;
	VkPipelineLayout m_LightingPipelineLayout;

	VkDescriptorSetLayout m_ToneMappingDescriptorSetLayout;
	VkPipeline m_ToneMappingPipeline;
	VkPipelineLayout m_ToneMappingPipelineLayout;

	VkPipeline m_DepthPrepassPipeline;
	VkPipelineLayout m_DepthPrepassPipelineLayout;

	std::vector<VkDescriptorSetLayoutBinding> m_Bindings{};
	std::vector<VkDescriptorBindingFlags> m_BindingFlags{};

	ResourceManager* m_ResourceManager;
	SwapChain* m_SwapChain;
	Device* m_Device;
};