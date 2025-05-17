#pragma once
#include "ResourceManager.h"
#include <stdexcept>
#include "Device.h"
#include "SwapChain.h"
#include "CommandManager.h"
#include "PipelineManager.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>
#include <iostream>
#include <filesystem>

void ResourceManager::CreateDepthResources(SwapChain* swapChain)
{
    VkFormat depthFormat = FindDepthFormat();
    CreateImage(swapChain->GetSwapChainExtent().width, swapChain->GetSwapChainExtent().height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);
    m_DepthImageView = CreateImageView(m_DepthImage.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    TransitionImageLayout(m_DepthImage, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                          VK_PIPELINE_STAGE_2_NONE,
                          VK_ACCESS_2_NONE,
                          VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                          VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    );
}


void ResourceManager::CreateTextureImage(const std::string& path, VkFormat format)
{

    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    else {
		std::cout << "loaded texture: " << path << std::endl;
    }

    m_Textures.push_back(Texture{});
    int currentTextureIndex = static_cast<int>(m_Textures.size() - 1);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_Device->GetDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_Device->GetDevice(), stagingBufferMemory);
    stbi_image_free(pixels);

    CreateImage(texWidth, texHeight, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Textures[currentTextureIndex].image, m_Textures[currentTextureIndex].imageMemory);

    TransitionImageLayout(m_Textures[currentTextureIndex].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_PIPELINE_STAGE_2_NONE,
                          VK_ACCESS_2_NONE,
                          VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                          VK_ACCESS_2_TRANSFER_WRITE_BIT
        );
    CopyBufferToImage(stagingBuffer, m_Textures[currentTextureIndex].image.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    TransitionImageLayout(m_Textures[currentTextureIndex].image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                          VK_ACCESS_2_TRANSFER_WRITE_BIT,
                          VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                          VK_ACCESS_2_SHADER_READ_BIT
        );

    m_Textures[currentTextureIndex].image.format = format;

    vkDestroyBuffer(m_Device->GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_Device->GetDevice(), stagingBufferMemory, nullptr);

}

void ResourceManager::CreateTextureImageView()
{
	for (auto& texture : m_Textures)
	{
		texture.imageView = CreateImageView(texture.image.image, texture.image.format, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void ResourceManager::CreateTextureSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_Device->GetPhysicalDevice(), &properties);

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

	for (auto& texture : m_Textures)
	{
		if (vkCreateSampler(m_Device->GetDevice(), &samplerInfo, nullptr, &texture.sampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}
}

void ResourceManager::CreateVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);


    void* data;
    vkMapMemory(m_Device->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_Vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(m_Device->GetDevice(), stagingBufferMemory);

    CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexBufferMemory);
    CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);
    vkDestroyBuffer(m_Device->GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_Device->GetDevice(), stagingBufferMemory, nullptr);
}

void ResourceManager::CreateMaterialBuffer()
{
    VkDeviceSize bufferSize = sizeof(GpuMaterial) * m_Meshes.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    void* data;
    vkMapMemory(m_Device->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    for (size_t i = 0; i < m_Meshes.size(); ++i) {
        memcpy(static_cast<char*>(data) + i * sizeof(m_Meshes[0].material), &m_Meshes[i].material, sizeof(m_Meshes[0].material));
    }
    vkUnmapMemory(m_Device->GetDevice(), stagingBufferMemory);

    CreateBuffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_MaterialBuffer,
        m_MaterialBufferMemory);

    CopyBuffer(stagingBuffer, m_MaterialBuffer, bufferSize);

    vkDestroyBuffer(m_Device->GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_Device->GetDevice(), stagingBufferMemory, nullptr);
}

void ResourceManager::CreateIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(m_Indices[0]) * m_Indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_Device->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_Indices.data(), (size_t)bufferSize);
    vkUnmapMemory(m_Device->GetDevice(), stagingBufferMemory);

    CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer, m_IndexBufferMemory);
    CopyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);
    vkDestroyBuffer(m_Device->GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_Device->GetDevice(), stagingBufferMemory, nullptr);
}

void ResourceManager::CreateUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    m_UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    m_UniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    m_UniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_UniformBuffers[i], m_UniformBuffersMemory[i]);

        vkMapMemory(m_Device->GetDevice(), m_UniformBuffersMemory[i], 0, bufferSize, 0, &m_UniformBuffersMapped[i]);
    }
}

void ResourceManager::CreateLightingUniformBuffer()
{
    VkDeviceSize bufferSize = sizeof(LightingUBO);

    CreateBuffer(bufferSize,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 m_LightingUniformBuffer,
                 m_LightingUniformBufferMemory);
    vkMapMemory(m_Device->GetDevice(), m_LightingUniformBufferMemory, 0, bufferSize, 0, &m_LightingUniformBufferMapped);

    LightingUBO lightingData{};
    lightingData.viewPos = glm::vec3(0.0f, 0.0f, 0.0f);
    lightingData.lightDir = glm::normalize(glm::vec3(0.577f, -0.577f, -0.577f));
    lightingData.lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    lightingData.ambientStrength = 0.1f;

    memcpy(m_LightingUniformBufferMapped, &lightingData, sizeof(LightingUBO));
}

void ResourceManager::CreateDescriptorPools() {

    uint32_t totalUniformBuffers = MAX_FRAMES_IN_FLIGHT + 1;
    uint32_t totalStorageBuffers = MAX_FRAMES_IN_FLIGHT * 2;
    uint32_t totalCombinedImageSamplers = (MAX_FRAMES_IN_FLIGHT + 5000) + 2;

    totalUniformBuffers += 2;
    totalStorageBuffers += 2;
    totalCombinedImageSamplers += 10;

    std::array<VkDescriptorPoolSize, 4> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = totalUniformBuffers;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[1].descriptorCount = totalStorageBuffers;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = totalStorageBuffers;
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[3].descriptorCount = totalCombinedImageSamplers;
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT + 5);
    
    if (vkCreateDescriptorPool(m_Device->GetDevice(), &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void ResourceManager::CreateDescriptorSets(PipelineManager* pipelineManager) {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, pipelineManager->GetDescriptorSetLayout());

    uint32_t variableDescriptorCount = static_cast<uint32_t>(m_Textures.size());

    VkDescriptorSetVariableDescriptorCountAllocateInfo variableCountInfo{};
    variableCountInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    variableCountInfo.descriptorSetCount = 1;
    variableCountInfo.pDescriptorCounts = &variableDescriptorCount;

    VkDescriptorSetAllocateInfo allocInfo{};

    m_DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.pNext = &variableCountInfo;
        allocInfo.descriptorPool = m_DescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layouts[i];

        if (vkAllocateDescriptorSets(m_Device->GetDevice(), &allocInfo, &m_DescriptorSets[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Uniform buffer descriptor
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_UniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        // Vertex buffer as storage buffer descriptor
        VkDescriptorBufferInfo vertexBufferInfo{};
        vertexBufferInfo.buffer = m_VertexBuffer;
        vertexBufferInfo.offset = 0;
        vertexBufferInfo.range = VK_WHOLE_SIZE;

        // Texture image descriptors
        std::vector<VkDescriptorImageInfo> imageInfos;
        for (const auto& tex : m_Textures) {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = tex.imageView;
            imageInfo.sampler = tex.sampler;
            imageInfos.push_back(imageInfo);
        }

        VkDescriptorBufferInfo materialBufferInfo{};
        materialBufferInfo.buffer = m_MaterialBuffer;
        materialBufferInfo.offset = 0;
        materialBufferInfo.range = VK_WHOLE_SIZE;

        std::array<VkWriteDescriptorSet, 4> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_DescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = m_DescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;  // Now binding 1
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &vertexBufferInfo;


        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = m_DescriptorSets[i];
        descriptorWrites[2].dstBinding = 2;  // Now binding 3
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &materialBufferInfo;

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = m_DescriptorSets[i];
        descriptorWrites[3].dstBinding = 3;  // Now binding 2
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[3].descriptorCount = static_cast<uint32_t>(m_Textures.size());
        descriptorWrites[3].pImageInfo = imageInfos.data();


        vkUpdateDescriptorSets(m_Device->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void ResourceManager::CreateLightingDescriptorSet(PipelineManager* pipelineManager)
{

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &pipelineManager->GetLightingDescriptorSetLayout();

    if (vkAllocateDescriptorSets(m_Device->GetDevice(), &allocInfo, &m_LightingDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate lighting  descriptor set!");
    }

    std::array<VkWriteDescriptorSet, 5> descriptorWrites{};

    VkDescriptorImageInfo albedoInfo{};
    albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    albedoInfo.imageView = m_GBuffer.albedoImageView;
    albedoInfo.sampler = m_GBuffer.sampler;

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = m_LightingDescriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].pImageInfo = &albedoInfo;

    VkDescriptorImageInfo normalInfo{};
    normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    normalInfo.imageView = m_GBuffer.normalImageView;
    normalInfo.sampler = m_GBuffer.sampler;
    
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = m_LightingDescriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].pImageInfo = &normalInfo;

    VkDescriptorImageInfo pbrInfo{};
    pbrInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    pbrInfo.imageView = m_GBuffer.pbrImageView;
    pbrInfo.sampler = m_GBuffer.sampler;

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = m_LightingDescriptorSet;
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[2].pImageInfo = &pbrInfo;

    VkDescriptorImageInfo depthInfo{};
    depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    depthInfo.imageView = m_DepthImageView;
    depthInfo.sampler = m_GBuffer.sampler;

    descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[3].dstSet = m_LightingDescriptorSet;
    descriptorWrites[3].dstBinding = 3; // New binding number
    descriptorWrites[3].dstArrayElement = 0;
    descriptorWrites[3].descriptorCount = 1;
    descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[3].pImageInfo = &depthInfo;

    VkDescriptorBufferInfo matrixBufferInfo{};
    matrixBufferInfo.buffer = m_UniformBuffers[0]; // Use first frame's UBO
    matrixBufferInfo.offset = 0;
    matrixBufferInfo.range = sizeof(UniformBufferObject);

    descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[4].dstSet = m_LightingDescriptorSet;
    descriptorWrites[4].dstBinding = 4;
    descriptorWrites[4].dstArrayElement = 0;
    descriptorWrites[4].descriptorCount = 1;
    descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[4].pBufferInfo = &matrixBufferInfo;

    vkUpdateDescriptorSets(m_Device->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void ResourceManager::RecreateResources(SwapChain* pSwapchain, PipelineManager* pPipelineManager)
{
    CleanDepth();
    CleanupGBuffer();

    CreateDepthResources(pSwapchain);

    CreateGBuffer(pSwapchain->GetSwapChainExtent());

    CleanupDescriptorPool();

    CreateDescriptorPools();

    CreateDescriptorSets(pPipelineManager);
    CreateLightingDescriptorSet(pPipelineManager);
}

void ResourceManager::CreateGBuffer(VkExtent2D extent)
{

    //albedo
    m_GBuffer.albedo.format = VK_FORMAT_R8G8B8A8_SRGB;
    CreateImage(extent.width,extent.height,
                m_GBuffer.albedo.format,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                m_GBuffer.albedo,m_GBuffer.albedoImageMemory);

    m_GBuffer.albedoImageView = CreateImageView(m_GBuffer.albedo.image,
                                                m_GBuffer.albedo.format,
                                                VK_IMAGE_ASPECT_COLOR_BIT);

    //normal
    m_GBuffer.normal.format = VK_FORMAT_R8G8B8A8_UNORM;
	CreateImage(extent.width, extent.height,
		        m_GBuffer.normal.format,
		        VK_IMAGE_TILING_OPTIMAL,
		        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		        m_GBuffer.normal, m_GBuffer.normalImageMemory);

	m_GBuffer.normalImageView = CreateImageView(m_GBuffer.normal.image,
		                                        m_GBuffer.normal.format,
		                                        VK_IMAGE_ASPECT_COLOR_BIT);
    m_GBuffer.normal.currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    m_GBuffer.pbr.format = VK_FORMAT_R8G8B8A8_UNORM;
    CreateImage(
        extent.width, extent.height,
        m_GBuffer.pbr.format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_GBuffer.pbr, m_GBuffer.pbrImageMemory
    );
    m_GBuffer.pbrImageView = CreateImageView(
        m_GBuffer.pbr.image,
        m_GBuffer.pbr.format,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

	m_GBuffer.depth = m_DepthImage;

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

    if (vkCreateSampler(m_Device->GetDevice(), &samplerInfo, nullptr, &m_GBuffer.sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create G-Buffer sampler!");
    }
}

void ResourceManager::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = m_CommandManager->BeginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0,0,0 };
    region.imageExtent = {
        width,
        height,
        1
    };
    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );
    m_CommandManager->EndSingleTimeCommands(commandBuffer);
}

void ResourceManager::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = m_CommandManager->BeginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    m_CommandManager->EndSingleTimeCommands(commandBuffer);
}

void ResourceManager::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_Device->GetDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_Device->GetDevice(), buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_Device->GetDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }
    vkBindBufferMemory(m_Device->GetDevice(), buffer, bufferMemory, 0);
}

void ResourceManager::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Image& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    if (vkCreateImage(m_Device->GetDevice(), &imageInfo, nullptr, &image.image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_Device->GetDevice(), image.image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    image.currentLayout = imageInfo.initialLayout;
	image.format = format;

    if (vkAllocateMemory(m_Device->GetDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }
    vkBindImageMemory(m_Device->GetDevice(), image.image, imageMemory, 0);
}

uint32_t ResourceManager::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_Device->GetPhysicalDevice(), &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");

}

void ResourceManager::CleanupGBuffer()
{
	vkDestroySampler(m_Device->GetDevice(), m_GBuffer.sampler, nullptr);

    vkDestroyImageView(m_Device->GetDevice(), m_GBuffer.albedoImageView, nullptr);
	vkDestroyImage(m_Device->GetDevice(), m_GBuffer.albedo.image, nullptr);
	vkFreeMemory(m_Device->GetDevice(), m_GBuffer.albedoImageMemory, nullptr);

	vkDestroyImageView(m_Device->GetDevice(), m_GBuffer.normalImageView, nullptr);
	vkDestroyImage(m_Device->GetDevice(), m_GBuffer.normal.image, nullptr);
	vkFreeMemory(m_Device->GetDevice(), m_GBuffer.normalImageMemory, nullptr);

    vkDestroyImageView(m_Device->GetDevice(), m_GBuffer.pbrImageView, nullptr);
    vkDestroyImage(m_Device->GetDevice(), m_GBuffer.pbr.image, nullptr);
    vkFreeMemory(m_Device->GetDevice(), m_GBuffer.pbrImageMemory, nullptr);

    //the depth is done somewhere else
}

ResourceManager::ResourceManager(Device* device)
    : m_Device(device)
{
}

ResourceManager::~ResourceManager()
{
	for (auto& texture : m_Textures)
	{
		vkDestroyImageView(m_Device->GetDevice(), texture.imageView, nullptr);
		vkDestroySampler(m_Device->GetDevice(), texture.sampler, nullptr);

        vkDestroyImage(m_Device->GetDevice(), texture.image.image, nullptr);
        vkFreeMemory(m_Device->GetDevice(), texture.imageMemory, nullptr);
	}


	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroyBuffer(m_Device->GetDevice(), m_UniformBuffers[i], nullptr);
		vkFreeMemory(m_Device->GetDevice(), m_UniformBuffersMemory[i], nullptr);
	}
	vkDestroyDescriptorPool(m_Device->GetDevice(), m_DescriptorPool, nullptr);

	vkDestroyBuffer(m_Device->GetDevice(), m_VertexBuffer, nullptr);
	vkFreeMemory(m_Device->GetDevice(), m_VertexBufferMemory, nullptr);

	vkDestroyBuffer(m_Device->GetDevice(), m_IndexBuffer, nullptr);
	vkFreeMemory(m_Device->GetDevice(), m_IndexBufferMemory, nullptr);

    vkDestroyBuffer(m_Device->GetDevice(), m_MaterialBuffer, nullptr);
    vkFreeMemory(m_Device->GetDevice(), m_MaterialBufferMemory, nullptr);

    CleanupGBuffer();
}

void ResourceManager::CleanDepth()
{
    vkDestroyImageView(m_Device->GetDevice(), m_DepthImageView, nullptr);
    vkDestroyImage(m_Device->GetDevice(), m_DepthImage.image, nullptr);
    vkFreeMemory(m_Device->GetDevice(), m_DepthImageMemory, nullptr);
}

void ResourceManager::SetCommandManager(CommandManager* commandManager)
{
    m_CommandManager = commandManager;
}

void ResourceManager::Create(SwapChain* swapChain, PipelineManager* pipelineManager)
{
    CreateDepthResources(swapChain);

    CreateGBuffer(swapChain->GetSwapChainExtent());

    int textureAmount{ 0 };

	// create texture here from the paths that the scene class has loaded
	for (const auto& path : m_TexturePaths)
	{
		CreateTextureImage(path.first,path.second);
        textureAmount++;
	}
    std::cout << textureAmount << " amount of textures loaded." << std::endl;
	//for (const auto& path : m_TexturePaths)
	//{
	//	std::cout << " texture: " << path << std::endl;
	//}

	CreateTextureImageView();
    CreateTextureSampler();

	// scene class should have loaded the vertex and index data

    CreateVertexPullingBuffer();
    CreateMaterialBuffer();
	CreateIndexBuffer();
	CreateUniformBuffers();
	CreateDescriptorPools();
    CreateDescriptorSets(pipelineManager);
    CreateLightingDescriptorSet(pipelineManager);
}

void ResourceManager::CleanupDescriptorPool()
{
    if (m_DescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_Device->GetDevice(), m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }
}

VkImageView ResourceManager::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(m_Device->GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view");
    }
    return imageView;
}


VkFormat ResourceManager::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_Device->GetPhysicalDevice(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

VkFormat ResourceManager::FindDepthFormat()
{
    return FindSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

void ResourceManager::TransitionImageLayout(Image& image, VkImageLayout newLayout, VkPipelineStageFlags2 srcStageMask,
                                                                                  VkPipelineStageFlags2 srcAccessMask,
                                                                                  VkPipelineStageFlags2 dstStageMask,
                                                                                  VkPipelineStageFlags2 dstAccessMask)
{
    VkCommandBuffer commandBuffer = m_CommandManager->BeginSingleTimeCommands();

    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.pNext = nullptr;
    barrier.oldLayout = image.currentLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image.image;

    // For depth images, set the correct aspect mask
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (image.format == VK_FORMAT_D32_SFLOAT_S8_UINT || image.format == VK_FORMAT_D24_UNORM_S8_UINT) {
            aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }

    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    barrier.srcStageMask = srcStageMask;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstStageMask = dstStageMask;
    barrier.dstAccessMask = dstAccessMask;

    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.pNext = nullptr;
    dependencyInfo.dependencyFlags = 0;
    dependencyInfo.memoryBarrierCount = 0;
    dependencyInfo.pMemoryBarriers = nullptr;
    dependencyInfo.bufferMemoryBarrierCount = 0;
    dependencyInfo.pBufferMemoryBarriers = nullptr;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

    m_CommandManager->EndSingleTimeCommands(commandBuffer);

    image.currentLayout = newLayout;
}
void ResourceManager::TransitionImageLayoutInline(VkCommandBuffer commandBuffer, Image& image, VkImageLayout newLayout, VkPipelineStageFlags2 srcStageMask, VkPipelineStageFlags2 srcAccessMask, VkPipelineStageFlags2 dstStageMask, VkPipelineStageFlags2 dstAccessMask)
{
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.pNext = nullptr;
    barrier.oldLayout = image.currentLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image.image;

    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if (image.format == VK_FORMAT_D32_SFLOAT ||
        image.format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
        image.format == VK_FORMAT_D24_UNORM_S8_UINT ||
        image.format == VK_FORMAT_D16_UNORM ||
        image.format == VK_FORMAT_X8_D24_UNORM_PACK32)
    {
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (image.format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
            image.format == VK_FORMAT_D24_UNORM_S8_UINT)
        {
            aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }

    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    barrier.srcStageMask = srcStageMask;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstStageMask = dstStageMask;
    barrier.dstAccessMask = dstAccessMask;

    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.pNext = nullptr;
    dependencyInfo.dependencyFlags = 0;
    dependencyInfo.memoryBarrierCount = 0;
    dependencyInfo.pMemoryBarriers = nullptr;
    dependencyInfo.bufferMemoryBarrierCount = 0;
    dependencyInfo.pBufferMemoryBarriers = nullptr;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

    image.currentLayout = newLayout;
}

void ResourceManager::CreateVertexPullingBuffer() {
    VkDeviceSize bufferSize = sizeof(Vertex) * m_Vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    void* data;
    vkMapMemory(m_Device->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_Vertices.data(), bufferSize);
    vkUnmapMemory(m_Device->GetDevice(), stagingBufferMemory);

    CreateBuffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_VertexBuffer,
        m_VertexBufferMemory);

    CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

    vkDestroyBuffer(m_Device->GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_Device->GetDevice(), stagingBufferMemory, nullptr);
}