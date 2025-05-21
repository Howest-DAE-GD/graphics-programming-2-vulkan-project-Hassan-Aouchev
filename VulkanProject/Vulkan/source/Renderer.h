#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#include <vector>

class Device;
class PipelineManager;
class SwapChain;
class CommandManager;
class ResourceManager;
class Instance;
class Renderer
{
public:
	Renderer(Device* device, PipelineManager* pipelineManager, SwapChain* swapChain, CommandManager* commandManager,ResourceManager* resourceManager, Instance* instance);
	~Renderer();

	void UpdatePushConstants(VkCommandBuffer commandBuffer);
	void UpdateUniformBuffer(uint32_t currentImage);
	void DrawFrame();
	void CreateSyncObjects();
	void RecreateSwapChain();

private:

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
		app->m_FramebufferResized = true;
	}

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void RenderDepthPrepass(VkCommandBuffer commandBuffer);
	void RenderGBufferPass(VkCommandBuffer commandBuffer);
	void RenderLightingPass(VkCommandBuffer commandBuffer);
	void RenderToneMapping(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void RecordDeferredCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_InFlightFences;
	bool m_FramebufferResized = false;

	uint32_t m_CurrentFrame = 0;

	Device* m_Device;

	PipelineManager* m_PipelineManager;
	SwapChain* m_SwapChain;
	CommandManager* m_CommandManager;
	ResourceManager* m_ResourceManager;
	Instance* m_Instance;
};