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

struct GpuMaterial {
    alignas(4) uint32_t baseColorTextureIndex;
    alignas(4) uint32_t normalTextureIndex;
    alignas(4) uint32_t metallicRoughnessTextureIndex;
    alignas(4) uint32_t useTextureFlags;
    alignas(4) uint32_t hasAlphaMask;
    alignas(4) uint32_t alphaTextureIndex;
    alignas(4) float alphaCutoff;
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
    alignas(8) glm::ivec2 resolution;
    alignas(16) glm::vec3 cameraPosition;
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
    uint32_t mipLevels = 1;
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

struct Texture {
    VkImageView imageView;
    VkSampler sampler;
    Image image;
    VkDeviceMemory imageMemory;
};

struct LightingSSBO {
    alignas(16) glm::vec4 position;
    alignas(16) glm::vec4 color;
    alignas(4) float lumen;
    alignas(4) float lux;
};

class Device;
class SwapChain;
class CommandManager;
class PipelineManager;
class ResourceManager
{
private:

    void CreateTextureImage(const std::string& path, VkFormat format, std::vector<Texture>& textureContainer);
    void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    void CreateTextureImageView(std::vector<Texture>& textureContainer);
    void CreateTextureSampler(std::vector<Texture>& textureContainer);
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

	std::vector<Texture> m_Textures;
    std::vector<Texture> m_AlphaTextures;

    VkBuffer m_VertexBuffer;
    VkDeviceMemory m_VertexBufferMemory;

    VkBuffer m_MaterialBuffer;
    VkDeviceMemory m_MaterialBufferMemory;

    VkBuffer m_LightingBuffer;
    VkDeviceMemory m_LightingBufferMemory;
    void* m_LightingUniformBufferMapped;

    VkBuffer m_IndexBuffer;
    VkDeviceMemory m_IndexBufferMemory;

    std::vector<VkBuffer> m_UniformBuffers;
    std::vector<VkDeviceMemory> m_UniformBuffersMemory;
    std::vector<void*> m_UniformBuffersMapped;

    VkDescriptorPool m_DescriptorPool;
    std::vector<VkDescriptorSet> m_UniversalDescriptorSets;
    std::vector<VkDescriptorSet> m_GBufferDescriptorSets;
    std::vector<VkDescriptorSet> m_DepthPrepassDescriptorSets;

    VkDescriptorSet m_LightingDescriptorSet;

	Device* m_Device;
	CommandManager* m_CommandManager;

    Image m_DepthImage;
    VkDeviceMemory m_DepthImageMemory;
    VkImageView m_DepthImageView;

    std::vector<PushConstantData> m_PushConstants;
    std::unordered_map<std::string, uint32_t> m_TextureLookup;
    std::vector <std::pair< std::string, VkFormat >> m_TexturePaths;

    std::unordered_map<std::string, uint32_t> m_AlphaTextureLookup;
    std::vector <std::pair< std::string, VkFormat >> m_AlphaTexturePaths;

    std::vector<MeshHandle> m_Meshes;
    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;
    std::vector<LightingSSBO> m_Lights;

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

    int AddAlphaTexture(const std::string path, VkFormat format)
    {
        auto it = m_AlphaTextureLookup.find(path);
        if (it != m_AlphaTextureLookup.end()) {
            return it->second;
        }

        uint32_t index = static_cast<uint32_t>(m_AlphaTexturePaths.size());
        m_AlphaTexturePaths.push_back(std::make_pair(path, format));
        m_AlphaTextureLookup[path] = index;
        return index;
    }

    int GetTextureAmount() const { return m_TexturePaths.size(); }
    int GetAlphaTextureAmount() const { return m_AlphaTexturePaths.size(); }
    void SetCommandManager(CommandManager* commandManager);

    void Create(SwapChain* swapChain, PipelineManager* pipelineManager);

	void SetModelMatrix(const glm::mat4& model,int index) {
		m_PushConstants[index].model = model;
	}

    void RecreateResources(SwapChain* pSwapchain,PipelineManager* pPipelineManager);
    
    std::vector<MeshHandle>& GetMeshes() { return m_Meshes; }
    std::vector<LightingSSBO>& GetLights() { return m_Lights; }
    void AddPointLight(glm::vec3 position, glm::vec3 color, float lumen, float lux);
    void AddDirectionalLight(glm::vec3 direction, glm::vec3 color, float lumen, float lux);
	std::vector<void*> GetUniformBuffersMapped() { return m_UniformBuffersMapped;}
	VkBuffer GetVertexBuffer() const { return m_VertexBuffer;}
    VkBuffer GetMaterialBuffer() const { return m_MaterialBuffer;}
	VkBuffer GetIndexBuffer() const { return m_IndexBuffer; }
    VkDescriptorSet GetUniversalDescriptorSet(size_t frameIndex) const {
        return m_UniversalDescriptorSets[frameIndex];
    }

    VkDescriptorSet GetGBufferDescriptorSet(size_t frameIndex) const {
        return m_GBufferDescriptorSets[frameIndex];
    }

    VkDescriptorSet GetDepthPrepassDescriptorSet(size_t frameIndex) const {
        return !m_DepthPrepassDescriptorSets.empty() ? m_DepthPrepassDescriptorSets[frameIndex] : VK_NULL_HANDLE;
    }
    VkDescriptorSet& GetLightingDescriptorSet() { return m_LightingDescriptorSet; }


    std::vector<PushConstantData>& GetPushConstants() { return m_PushConstants; }

	std::vector<Vertex>& GetVertices() { return m_Vertices; }
	std::vector<uint32_t>& GetIndices() { return m_Indices; }
	VkImageView GetDepthImageView() const { return m_DepthImageView; }
    Image& GetDepthImage() { return m_DepthImage; }
    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1);
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

    bool HasAlphaTextures() { return !m_AlphaTextures.empty(); }

    void CleanDepth();
};