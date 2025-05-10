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

struct GBuffer {
    // G-Buffer images
    Image albedo;
    Image normal;
    Image depth;
    // i can add more if needed

    // Image views
    VkImageView albedoImageView;
    VkImageView normalImageView;
    VkImageView depthImageView;

    // Memory allocations
    VkDeviceMemory albedoImageMemory;
    VkDeviceMemory normalImageMemory;
    VkDeviceMemory depthImageMemory;

    // Sampler (can be shared)
    VkSampler sampler;
};

struct Vertex {
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 color;
    alignas(8) glm::vec2 texCoord;

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }
    static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }
    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
    
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

struct UniformBufferObject {
    glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct PushConstantData {
    glm::mat4 model;
    alignas(16) int textureIndex;
};

struct MeshHandle {
    uint32_t indexOffset;
    uint32_t indexCount;
    int textureIndex;
	glm::mat4 modelMatrix;
};

struct Image {
	VkImage image;
	VkImageLayout currentLayout;
	VkFormat format;
};

class Device;
class SwapChain;
class CommandManager;
class PipelineManager;
class ResourceManager
{
private:

    void CreateTextureImage(const std::string& path);
    void CreateTextureImageView();
    void CreateTextureSampler();
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateUniformBuffers();

    void CreateDescriptorPools();
    void CreateDescriptorSets(PipelineManager* pipelineManager);

    void CreateGBuffer(VkExtent2D extent);

    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Image& image, VkDeviceMemory& imageMemory);
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);


    struct Texture {
        VkImageView imageView;
        VkSampler sampler;
        Image image;
        VkDeviceMemory imageMemory;
    };
	std::vector<Texture> m_Textures;

    VkBuffer m_VertexBuffer;
    VkDeviceMemory m_VertexBufferMemory;
    VkBuffer m_IndexBuffer;
    VkDeviceMemory m_IndexBufferMemory;

    std::vector<VkBuffer> m_UniformBuffers;
    std::vector<VkDeviceMemory> m_UniformBuffersMemory;
    std::vector<void*> m_UniformBuffersMapped;

    VkDescriptorPool m_DescriptorPool;
    std::vector<VkDescriptorSet> m_DescriptorSets;

	Device* m_Device;
	CommandManager* m_CommandManager;

    Image m_DepthImage;
    VkDeviceMemory m_DepthImageMemory;
    VkImageView m_DepthImageView;

    std::vector<PushConstantData> m_PushConstants;
    std::unordered_map<std::string, uint32_t> m_TextureLookup;
    std::vector<std::string> m_TexturePaths;

    std::vector<MeshHandle> m_Meshes;
    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;

    GBuffer m_GBuffer;

    const int MAX_FRAMES_IN_FLIGHT = 2;
public:
    ResourceManager(Device* device);

	~ResourceManager();

    void CleanDepth();

    void AddModel(MeshHandle meshHandle) { 
		MeshHandle newMeshHandle = meshHandle;
        m_Meshes.push_back(meshHandle);
		m_PushConstants.push_back({ meshHandle.modelMatrix, meshHandle.textureIndex+1 });
    }
    int AddTexture(const std::string path) { 
        auto it = m_TextureLookup.find(path);
        if (it != m_TextureLookup.end()) {
            return it->second;
        }

        uint32_t index = static_cast<uint32_t>(m_TexturePaths.size());
        m_TexturePaths.push_back(path);
        m_TextureLookup[path] = index;
        return index;
}

    int GetTextureAmount() const { return m_TexturePaths.size()+1; }
    void SetCommandManager(CommandManager* commandManager);

    void Create(SwapChain* swapChain, PipelineManager* pipelineManager);

	void SetModelMatrix(const glm::mat4& model,int index) {
		m_PushConstants[index].model = model;
	}

    std::vector<MeshHandle> GetMeshes() const { return m_Meshes; }
	std::vector<void*> GetUniformBuffersMapped() { return m_UniformBuffersMapped; }
	VkBuffer GetVertexBuffer() const { return m_VertexBuffer; }
	VkBuffer GetIndexBuffer() const { return m_IndexBuffer; }
	std::vector<VkDescriptorSet>& GetDescriptorSets() { return m_DescriptorSets; }

    std::vector<PushConstantData>& GetPushConstants() { return m_PushConstants; }

	std::vector<Vertex>& GetVertices() { return m_Vertices; }
	std::vector<uint32_t>& GetIndices() { return m_Indices; }
	VkImageView GetDepthImageView() const { return m_DepthImageView; }
    Image& GetDepthImage() { return m_DepthImage; }
    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void CreateDepthResources(SwapChain* swapChain);
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat FindDepthFormat();
    void TransitionImageLayout(Image& image, VkImageLayout newLayout, VkPipelineStageFlags2 srcStageMask,
                                                                     VkPipelineStageFlags2 srcAccessMask,
                                                                     VkPipelineStageFlags2 dstStageMask,
                                                                     VkPipelineStageFlags2 dstAccessMask);
    void CreateVertexPullingBuffer();
};