#pragma once
#include "PipelineManager.h"
#include <array>
#include <stdexcept>
#include <iostream>
#include "SwapChain.h"
#include "ResourceManager.h"
#include "Device.h"

PipelineManager::PipelineManager(Device* device, ResourceManager* resourceManager, SwapChain* swapChain) : m_Device(device),
m_ResourceManager(resourceManager),
m_SwapChain(swapChain)
{
    CreateRenderPass();
    CreateDescriptorSetLayout();
    CreateGraphicsPipeline();
}

PipelineManager::~PipelineManager()
{
    vkDestroyDescriptorSetLayout(m_Device->GetDevice(), m_DescriptorSetLayout, nullptr);

    SavePipelineCache();
	vkDestroyPipelineCache(m_Device->GetDevice(), m_PipelineCache, nullptr);
	vkDestroyPipeline(m_Device->GetDevice(), m_GraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_Device->GetDevice(), m_PipelineLayout, nullptr);
	vkDestroyRenderPass(m_Device->GetDevice(), m_RenderPass, nullptr);
}

void PipelineManager::CreateRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_SwapChain->GetSwapChainImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = m_ResourceManager->FindDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentReference{};
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentReference;


    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;

    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;


    if (vkCreateRenderPass(m_Device->GetDevice(), &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}
void PipelineManager::CreateDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;

    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    uint32_t textureAmount = m_ResourceManager->GetTextureAmount();

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = textureAmount;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2>bindings = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(m_Device->GetDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void PipelineManager::CreateGraphicsPipeline()
{
    auto vertShaderCode = readFile("CustomShaders/shader.vert.spv");
    auto fragShaderCode = readFile("CustomShaders/shader.frag.spv");

    VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

    struct SpecializationData {
        uint32_t maxTextureCount;
    } specializationData;

    specializationData.maxTextureCount = m_ResourceManager->GetTextureAmount();

    //=======================================================================
    // !!!! IMPORTANT QUESTION FOR TEACHER !!!!
    //=======================================================================
    // why does this code work on two computers but not the third?
    // 
    // the issue: In descriptor pool creation i only allocate MAX_FRAMES_IN_FLIGHT
    // descriptors (2-3), but in the shader i declared an array of 64 textures.
    // 
    // specialization constant fixes it by setting the exact texture count:
    //=======================================================================

    VkSpecializationMapEntry specializationMapEntry{};
    specializationMapEntry.constantID = 0;  
    specializationMapEntry.offset = offsetof(SpecializationData, maxTextureCount);
    specializationMapEntry.size = sizeof(uint32_t);

    VkSpecializationInfo specializationInfo{};
    specializationInfo.mapEntryCount = 1;
    specializationInfo.pMapEntries = &specializationMapEntry;
    specializationInfo.dataSize = sizeof(SpecializationData);
    specializationInfo.pData = &specializationData;

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";
    fragShaderStageInfo.pSpecializationInfo = &specializationInfo;

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo,fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::GetBindingDescription();
    auto attributeDescriptions = Vertex::GetAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthBiasClamp = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    //optional depth bound test.
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;

    //configure stencil buffer operations
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};

    VkPipelineColorBlendAttachmentState colorBlendAttachement;
    colorBlendAttachement.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachement.blendEnable = VK_FALSE;
    // the following is optional but not used if blendEnable is false maybe later change is needed
    colorBlendAttachement.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachement.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachement.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachement.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachement.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachement.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachement;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstantData);

    std::vector <VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(m_Device->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    std::vector<uint8_t> initialCacheData;

    VkPipelineCacheCreateInfo CacheCreateInfo{};
    CacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;


    std::string readFileName = "pipeline_cache_data.bin";
    FILE* pReadFile = fopen(readFileName.c_str(), "rb");

    if (pReadFile) {
        fseek(pReadFile, 0, SEEK_END);
        long fileSize = ftell(pReadFile);
        rewind(pReadFile);

        initialCacheData.resize(fileSize);
        if (fread(initialCacheData.data(), 1, fileSize, pReadFile) != fileSize) {
            std::cerr << "Failed to read pipeline cache file" << std::endl;
            initialCacheData.clear();
        }

        fclose(pReadFile);
        printf(" Pipeline cache HIT!\n");
        printf(" cacheData loaded from %s\n", readFileName.c_str());
    }
    else
    {
        printf(" Pipeline cache miss!\n");
    }

    if (!initialCacheData.empty()) {
        VkPipelineCacheHeaderVersionOne* header =
            reinterpret_cast<VkPipelineCacheHeaderVersionOne*>(initialCacheData.data());

        bool isValideCache =
            header->headerSize == sizeof(VkPipelineCacheHeaderVersionOne) &&
            header->headerVersion == VK_PIPELINE_CACHE_HEADER_VERSION_ONE;

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(m_Device->GetPhysicalDevice(), &deviceProperties);

        isValideCache &= (header->vendorID == deviceProperties.vendorID);
        isValideCache &= (header->deviceID == deviceProperties.deviceID);

        if (!isValideCache) {
            std::cout << "existing pipeline cache is invalid. Creating new cache" << std::endl;
            initialCacheData.clear();
        }
    }

    CacheCreateInfo.initialDataSize = initialCacheData.size();
    CacheCreateInfo.pInitialData = initialCacheData.empty() ? nullptr : initialCacheData.data();

    if (vkCreatePipelineCache(m_Device->GetDevice(), &CacheCreateInfo, nullptr, &m_PipelineCache) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline cache!");
    }

    VkFormat colorFormat = m_SwapChain->GetSwapChainImageFormat();

	VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
	pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	pipelineRenderingCreateInfo.pNext = VK_NULL_HANDLE;
	pipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat;
	pipelineRenderingCreateInfo.depthAttachmentFormat = m_ResourceManager->FindDepthFormat();
	pipelineRenderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = &pipelineRenderingCreateInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.pDepthStencilState = &depthStencil;

    pipelineInfo.layout = m_PipelineLayout;

    pipelineInfo.renderPass = nullptr; // used m_RenderPassm_RenderPass
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = 1;

    if (vkCreateGraphicsPipelines(m_Device->GetDevice(), m_PipelineCache, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline");
    }

    vkDestroyShaderModule(m_Device->GetDevice(), fragShaderModule, nullptr);
    vkDestroyShaderModule(m_Device->GetDevice(), vertShaderModule, nullptr);
}

void PipelineManager::SavePipelineCache()
{
    size_t cacheSize;
    VkResult result = vkGetPipelineCacheData(m_Device->GetDevice(), m_PipelineCache, &cacheSize, nullptr);

    if (result != VK_SUCCESS) {
        std::cerr << "Failed to query pipeline cache size: " << result << std::endl;
        return;
    }

    std::vector<uint8_t> cacheData(cacheSize);
    result = vkGetPipelineCacheData(m_Device->GetDevice(), m_PipelineCache, &cacheSize, cacheData.data());

    if (result != VK_SUCCESS) {
        std::cerr << "Failed to retrieve pipeline cache data: " << result << std::endl;
        return;
    }
    const std::string cacheFileName = "pipeline_cache_data.bin";
    FILE* outputFile = fopen(cacheFileName.c_str(), "wb");
    if (!outputFile) {
        std::cerr << "Failed to open pipeline cache file for writing: " << cacheFileName << std::endl;
        return;
    }
    size_t written = fwrite(cacheData.data(), 1, cacheSize, outputFile);

    fclose(outputFile);

    if (written != cacheSize) {
        std::cerr << "Failed to write complete pipeline cache. " << "Wrote " << written << " of " << cacheSize << " bytes." << std::endl;
    }
    else {
        std::cout << "Successfully saved pipeline cache to " << cacheFileName << " (" << cacheSize << " bytes)" << std::endl;
    }
}

VkShaderModule PipelineManager::CreateShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_Device->GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    return shaderModule;
}