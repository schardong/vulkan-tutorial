#ifndef __VULKAN_PROG__
#define __VULKAN_PROG__

#include <vulkan/vulkan.hpp>


struct GLFWwindow;
struct QueueFamilyIndices;
struct SwapChainSupportDetails;
struct Vertex;


class VulkanProg
{
public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

	void setFramebufferResized(bool resized)
	{
		m_framebuffer_resized = resized;
	}

private:
	void initVulkan();
	void initWindow();
	void mainLoop();
	void cleanup();

	bool checkValidationlayerSupport();
	std::vector<const char*> getRequiredExtensions();
	void setupDebugCb();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createGraphicsPipeline();
	void createFramebuffers();
	void createCommandPool();
	void createCommandBuffers();
	void createSyncObjects();
	void drawFrame();
	void createVertexBuffer();
	void createIndexBuffer();
	void createTextureImage();
	void createTextureSampler();
	void createDescriptorSetLayout();
	void createUniformBuffer();
	void createDescriptorPool();
	void createDescriptorSets();
	void updateUniformBuffer(uint32_t image_index);

	void cleanupSwapChain();
	void rebuildSwapChain();

	VkShaderModule createShaderModule(const std::vector<char>& bytecode);
	bool isDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

private:
	GLFWwindow* m_window;
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debug_messenger;
	VkPhysicalDevice m_device = VK_NULL_HANDLE;
	VkDevice m_logical_device;
	VkQueue m_graphics_queue;
	VkQueue m_presentation_queue;
	VkSurfaceKHR m_surface;
	VkSwapchainKHR m_swapchain;
	VkFormat m_swapchain_format;
	VkExtent2D m_swapchain_extent;
	std::vector<VkImage> m_swapchain_images;
	std::vector<VkImageView> m_swapchain_image_views;
	VkRenderPass m_renderpass;
	VkDescriptorSetLayout m_descriptor_set_layout;
	VkPipelineLayout m_pipeline_layout;
	VkPipeline m_graphics_pipeline;
	std::vector<VkFramebuffer> m_swapchain_framebuffers;
	VkCommandPool m_command_pool;
	std::vector<VkCommandBuffer> m_command_buffers;
	std::vector<VkSemaphore> m_image_available_semaphores;
	std::vector<VkSemaphore> m_render_finished_semaphores;
	std::vector<VkFence> m_inflight_fences;
	size_t m_current_frame = 0;
	VkBuffer m_vertex_buffer;
	VkDeviceMemory m_vertex_buffer_memory;
	VkBuffer m_index_buffer;
	VkDeviceMemory m_index_buffer_memory;
	std::vector<VkBuffer> m_uniform_buffers;
	std::vector<VkDeviceMemory> m_uniform_buffer_memories;
	VkDescriptorPool m_descriptor_pool;
	std::vector<VkDescriptorSet> m_descriptor_sets;
	VkImage m_texture_image;
	VkDeviceMemory m_texture_image_memory;
	VkImageView m_texture_image_view;
	VkSampler m_texture_sampler;

	const int WIDTH = 800;
	const int HEIGHT = 600;
	bool m_framebuffer_resized = false;
};


const std::vector<const char*> validation_layers = {
	"VK_LAYER_LUNARG_standard_validation"
};


const std::vector<const char*> device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


const int MAX_FRAMES_IN_FLIGHT = 2;


#endif  __VULKAN_PROG__