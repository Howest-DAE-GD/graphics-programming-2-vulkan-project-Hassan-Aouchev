#pragma once
#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <string>
#include <iostream>

struct alignas(16) GpuMaterial {
    uint32_t baseColorTextureIndex;
    uint32_t normalTextureIndex;
    uint32_t metallicRoughnessTextureIndex;
    uint32_t useTextureFlags;

};

struct Vertex {
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 normal;
    alignas(8) glm::vec2 texCoord;
    alignas(16) glm::vec3 tangent;
    alignas(16) glm::vec3 bitTangent;

    bool operator==(const Vertex& other) const {
        return pos == other.pos &&
            normal == other.normal &&
            texCoord == other.texCoord &&
            tangent == other.tangent &&
            bitTangent == other.bitTangent;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(const Vertex& vertex) const {
            size_t seed = 0;
            auto hasherVec3 = hash<glm::vec3>();
            auto hasherVec2 = hash<glm::vec2>();

            seed ^= hasherVec3(vertex.pos) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hasherVec3(vertex.normal) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hasherVec2(vertex.texCoord) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hasherVec3(vertex.tangent) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hasherVec3(vertex.bitTangent) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

            return seed;
        }
    };
}

struct UniformBufferObject {
    glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct PushConstantData {
    glm::mat4 model;
    alignas(16) int meshIndex;
};

struct MeshHandle {
    uint32_t indexOffset;
    uint32_t indexCount;
    GpuMaterial material;
	glm::mat4 modelMatrix;
};

struct Image {
	VkImage image;
	VkImageLayout currentLayout;
	VkFormat format;
};
struct GBuffer {
    // G-Buffer images
    Image albedo;
    Image normal;
    Image pbr;
    Image depth;
    // i can add more if needed

    // Image views
    VkImageView albedoImageView;
    VkImageView normalImageView;
    VkImageView pbrImageView;
    VkImageView depthImageView;

    // Memory allocations
    VkDeviceMemory albedoImageMemory;
    VkDeviceMemory normalImageMemory;
    VkDeviceMemory pbrImageMemory;

    // Sampler (can be shared)
    VkSampler sampler;
};

struct LightingUBO {
    alignas(16)glm::vec3 viewPos;
    alignas(16)glm::vec3 lightDir;
    alignas(16)glm::vec3 lightColor;
    alignas(16)float ambientStrength;
};

class Device;
class SwapChain;
class CommandManager;
class PipelineManager;
class ResourceManager
{
private:

    void CreateTextureImage(const std::string& path, VkFormat format);
    void CreateTextureImageView();
    void CreateTextureSampler();
    void CreateVertexBuffer();
    void CreateMaterialBuffer();
    void CreateIndexBuffer();
    void CreateUniformBuffers();
    void CreateLightingUniformBuffer();
    void CreateGBuffer(VkExtent2D extent);
    void CreateDescriptorSets(PipelineManager* pipelineManager);
    void CreateLightingDescriptorSet(PipelineManager* pipelineManager);
    void CreateDescriptorPools();
    void CreateDepthResources(SwapChain* swapChain);



    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Image& image, VkDeviceMemory& imageMemory);
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void CleanupGBuffer();
    void CleanupDescriptorPool();

    struct Texture {
        VkImageView imageView;
        VkSampler sampler;
        Image image;
        VkDeviceMemory imageMemory;
    };
	std::vector<Texture> m_Textures;

    VkBuffer m_VertexBuffer;
    VkDeviceMemory m_VertexBufferMemory;

    VkBuffer m_MaterialBuffer;
    VkDeviceMemory m_MaterialBufferMemory;

    VkBuffer m_LightingUniformBuffer;
    VkDeviceMemory m_LightingUniformBufferMemory;
    void* m_LightingUniformBufferMapped;

    VkBuffer m_IndexBuffer;
    VkDeviceMemory m_IndexBufferMemory;

    std::vector<VkBuffer> m_UniformBuffers;
    std::vector<VkDeviceMemory> m_UniformBuffersMemory;
    std::vector<void*> m_UniformBuffersMapped;

    VkDescriptorPool m_DescriptorPool;
    std::vector<VkDescriptorSet> m_DescriptorSets;

    VkDescriptorSet m_LightingDescriptorSet;

	Device* m_Device;
	CommandManager* m_CommandManager;

    Image m_DepthImage;
    VkDeviceMemory m_DepthImageMemory;
    VkImageView m_DepthImageView;

    std::vector<PushConstantData> m_PushConstants;
    std::unordered_map<std::string, uint32_t> m_TextureLookup;
    std::vector <std::pair< std::string, VkFormat >> m_TexturePaths;

    std::vector<MeshHandle> m_Meshes;
    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;

    GBuffer m_GBuffer;

    const int MAX_FRAMES_IN_FLIGHT = 2;
public:
    ResourceManager(Device* device);

	~ResourceManager();


    GBuffer& GetGBuffer() { return m_GBuffer; }

    void AddModel(MeshHandle meshHandle,int meshIndex) { 
		MeshHandle newMeshHandle = meshHandle;
        m_Meshes.push_back(meshHandle);
		m_PushConstants.push_back({ meshHandle.modelMatrix, meshIndex});
    }
    int AddTexture(const std::string path,VkFormat format) { 
        auto it = m_TextureLookup.find(path);
        if (it != m_TextureLookup.end()) {
            return it->second;
        }

        uint32_t index = static_cast<uint32_t>(m_TexturePaths.size());
        m_TexturePaths.push_back(std::make_pair(path,format));
        m_TextureLookup[path] = index;
        return index;
}

    int GetTextureAmount() const { return m_TexturePaths.size(); }
    void SetCommandManager(CommandManager* commandManager);

    void Create(SwapChain* swapChain, PipelineManager* pipelineManager);

	void SetModelMatrix(const glm::mat4& model,int index) {
		m_PushConstants[index].model = model;
	}

    void RecreateResources(SwapChain* pSwapchain,PipelineManager* pPipelineManager);
    
    std::vector<MeshHandle>& GetMeshes() { return m_Meshes; }
	std::vector<void*> GetUniformBuffersMapped() { return m_UniformBuffersMapped;}
	VkBuffer GetVertexBuffer() const { return m_VertexBuffer;}
    VkBuffer GetMaterialBuffer() const { return m_MaterialBuffer;}
	VkBuffer GetIndexBuffer() const { return m_IndexBuffer; }
	std::vector<VkDescriptorSet>& GetDescriptorSets() { return m_DescriptorSets; }
    VkDescriptorSet& GetLightingDescriptorSet() { return m_LightingDescriptorSet; }


    std::vector<PushConstantData>& GetPushConstants() { return m_PushConstants; }

	std::vector<Vertex>& GetVertices() { return m_Vertices; }
	std::vector<uint32_t>& GetIndices() { return m_Indices; }
	VkImageView GetDepthImageView() const { return m_DepthImageView; }
    Image& GetDepthImage() { return m_DepthImage; }
    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat FindDepthFormat();
    void TransitionImageLayout(Image& image, VkImageLayout newLayout, VkPipelineStageFlags2 srcStageMask,
                                                                     VkPipelineStageFlags2 srcAccessMask,
                                                                     VkPipelineStageFlags2 dstStageMask,
                                                                     VkPipelineStageFlags2 dstAccessMask);
    void TransitionImageLayoutInline(VkCommandBuffer commandBuffer, Image& image, VkImageLayout newLayout,
                                     VkPipelineStageFlags2 srcStageMask,
                                     VkPipelineStageFlags2 srcAccessMask,
                                     VkPipelineStageFlags2 dstStageMask,
                                     VkPipelineStageFlags2 dstAccessMask);
    void CreateVertexPullingBuffer();

    void CleanDepth();
};