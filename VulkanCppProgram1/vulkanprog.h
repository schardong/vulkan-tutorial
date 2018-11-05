#ifndef __VULKAN_PROG__
#define __VULKAN_PROG__

#include <vulkan/vulkan.hpp>


struct GLFWwindow;
struct QueueFamilyIndices;


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
	bool isDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice dev);

private:
	GLFWwindow* m_window;
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debug_messenger;
	VkPhysicalDevice m_device = VK_NULL_HANDLE;
	VkDevice m_logical_device;
	VkQueue m_graphics_queue;
	VkQueue m_presentation_queue;
	VkSurfaceKHR m_surface;

	const int WIDTH = 800;
	const int HEIGHT = 600;
};


const std::vector<const char*> validation_layers = {
	"VK_LAYER_LUNARG_standard_validation"
};


#endif  __VULKAN_PROG__