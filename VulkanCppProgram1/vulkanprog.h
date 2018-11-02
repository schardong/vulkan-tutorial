#ifndef __VULKAN_PROG__
#define __VULKAN_PROG__

#include <vulkan/vulkan.hpp>
#include <stdexcept>
#include <iostream>
#include <functional>
#include <cstdlib>


struct GLFWwindow;


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

private:
	GLFWwindow* m_window;
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debug_messenger;

	const int WIDTH = 800;
	const int HEIGHT = 600;
};


const std::vector<const char*> validation_layers = {
	"VK_LAYER_LUNARG_standard_validation"
};


#endif  __VULKAN_PROG__