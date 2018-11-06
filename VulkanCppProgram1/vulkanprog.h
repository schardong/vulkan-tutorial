#ifndef __VULKAN_PROG__
#define __VULKAN_PROG__

#include <vulkan/vulkan.hpp>


struct GLFWwindow;
struct QueueFamilyIndices;
struct SwapChainSupportDetails;


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
	VkPipelineLayout m_pipeline_layout;

	const int WIDTH = 800;
	const int HEIGHT = 600;
};


const std::vector<const char*> validation_layers = {
	"VK_LAYER_LUNARG_standard_validation"
};


const std::vector<const char*> device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


#endif  __VULKAN_PROG__