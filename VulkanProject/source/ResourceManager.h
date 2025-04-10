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

const std::string MODEL_PATH = "model/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

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
    MeshHandle LoadModel(const std::string& path, int textureIndex, glm::vec3 position);
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateUniformBuffers();

    void CreateDescriptorPools();
    void CreateDescriptorSets(PipelineManager* pipelineManager);

    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    std::vector<MeshHandle> m_Meshes;

    struct Texture {
        VkImageView imageView;
        VkSampler sampler;
        VkImage image;
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

    VkImage m_DepthImage;
    VkDeviceMemory m_DepthImageMemory;
    VkImageView m_DepthImageView;

    std::vector<PushConstantData> m_PushConstants;

    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;

    const int MAX_FRAMES_IN_FLIGHT = 2;
public:
    ResourceManager(Device* device);

	~ResourceManager();

    void CleanDepth();

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

	std::vector<Vertex> GetVertices() const { return m_Vertices; }
	std::vector<uint32_t> GetIndices() const { return m_Indices; }
	VkImageView GetDepthImageView() const { return m_DepthImageView; }
    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void CreateDepthResources(SwapChain* swapChain);
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat FindDepthFormat();
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
};