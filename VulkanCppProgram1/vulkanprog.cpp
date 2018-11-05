#include "vulkanprog.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <stdexcept>
#include <iostream>
#include <functional>
#include <cstdlib>
#include <optional>
#include <set>

#ifdef NDEBUG
bool enable_validation_layer = false;
#else
bool enable_validation_layer = true;
#endif // NDEBUG


struct QueueFamilyIndices
{
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;

	bool isComplete()
	{
		return graphics_family.has_value() && present_family.has_value();
	}
};


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCb(
	VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity,
	VkDebugUtilsMessageTypeFlagsEXT msg_type,
	const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	void* user_data)
{
	std::cerr << "Validation layer: " << callback_data->pMessage << std::endl;
	return VK_FALSE;
}


VkResult createDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* create_info,
	const VkAllocationCallbacks* allocator,
	VkDebugUtilsMessengerEXT* callback)
{
	auto fn = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (fn) {
		return fn(instance, create_info, allocator, callback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	return VK_SUCCESS;
}


VkResult destroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT callback,
	const VkAllocationCallbacks* allocator)
{
	auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (fn) {
		fn(instance, callback, allocator);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	return VK_SUCCESS;
}


// Class methods below.
void VulkanProg::initVulkan()
{
	if (enable_validation_layer && !checkValidationlayerSupport())
		throw std::runtime_error("Required validation layers not found.");

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Basic triangle";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "No engine";
	app_info.apiVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	auto req_extensions = getRequiredExtensions();

	VkInstanceCreateInfo instance_info = {};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app_info;
	instance_info.enabledExtensionCount = static_cast<uint32_t>(req_extensions.size());
	instance_info.ppEnabledExtensionNames = req_extensions.data();

	if (enable_validation_layer) {
		instance_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		instance_info.ppEnabledLayerNames = validation_layers.data();
	}
	else {
		instance_info.enabledLayerCount = 0;
	}

	if (vkCreateInstance(&instance_info, nullptr, &m_instance) != VK_SUCCESS)
		throw std::runtime_error("Failed to create Vulkan instance.");

	setupDebugCb();

	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
		throw std::runtime_error("Failed to create window surface.");

	pickPhysicalDevice();
	createLogicalDevice();
}

void VulkanProg::initWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_window = glfwCreateWindow(WIDTH, HEIGHT, "Basic triangle with Vulkan", nullptr, nullptr);
}

void VulkanProg::mainLoop()
{
	while (!glfwWindowShouldClose(m_window))
		glfwPollEvents();
}

void VulkanProg::cleanup()
{
	vkDestroyDevice(m_logical_device, nullptr);
	if (enable_validation_layer) {
		destroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
	}
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

bool VulkanProg::checkValidationlayerSupport()
{
	uint32_t count = 0;

	vkEnumerateInstanceLayerProperties(&count, nullptr);
	std::vector<VkLayerProperties> available_layers(count);
	vkEnumerateInstanceLayerProperties(&count, available_layers.data());

	for (const char* layer : validation_layers) {
		bool found = false;

		for (const auto& layer_props : available_layers) {
			if (strcmp(layer, layer_props.layerName) == 0) {
				found = true;
				break;
			}
		}

		if (!found)
			return false;
	}

	return true;
}

std::vector<const char*> VulkanProg::getRequiredExtensions()
{
	uint32_t glfw_ext_count = 0;
	const char** glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

	std::vector<const char*> extensions(glfw_ext, glfw_ext + glfw_ext_count);
	if (enable_validation_layer)
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return extensions;
}

void VulkanProg::setupDebugCb()
{
	if (!enable_validation_layer)
		return;

	VkDebugUtilsMessengerCreateInfoEXT create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = debugCb;
	create_info.pUserData = nullptr;

	if (createDebugUtilsMessengerEXT(m_instance, &create_info, nullptr, &m_debug_messenger) != VK_SUCCESS)
		throw std::runtime_error("Failed to create debug callback.");
}

void VulkanProg::pickPhysicalDevice()
{
	uint32_t count = 0;
	vkEnumeratePhysicalDevices(m_instance, &count, nullptr);

	if (!count)
		throw std::runtime_error("No Vulkan capable physical devices.");

	std::vector<VkPhysicalDevice> devices(count);
	vkEnumeratePhysicalDevices(m_instance, &count, devices.data());
	
	for (const auto& device : devices) {
		if (isDeviceSuitable(device)) {
			m_device = device;
			break;
		}
	}

	if (m_device == VK_NULL_HANDLE)
		throw std::runtime_error("No suitable Vulkan devices found.");
}

void VulkanProg::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(m_device);

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<uint32_t> unique_queue_families = { indices.graphics_family.value(), indices.present_family.value() };

	float queue_priority = 1.f;
	for (uint32_t queue_family : unique_queue_families) {
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;
		queue_create_infos.push_back(queue_create_info);
	}

	VkPhysicalDeviceFeatures dev_features = {};

	VkDeviceCreateInfo device_create_info = {};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.pQueueCreateInfos = queue_create_infos.data();
	device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	device_create_info.pEnabledFeatures = &dev_features;
	device_create_info.enabledExtensionCount = 0;

	if (enable_validation_layer) {
		device_create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		device_create_info.ppEnabledLayerNames = validation_layers.data();
	}
	else {
		device_create_info.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_device, &device_create_info, nullptr, &m_logical_device) != VK_SUCCESS)
		throw std::runtime_error("Failed to create logical device.");

	vkGetDeviceQueue(m_logical_device, indices.graphics_family.value(), 0, &m_graphics_queue);
	vkGetDeviceQueue(m_logical_device, indices.present_family.value(), 0, &m_presentation_queue);
}

bool VulkanProg::isDeviceSuitable(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties dev_properties;
	vkGetPhysicalDeviceProperties(device, &dev_properties);

	//VkPhysicalDeviceFeatures dev_features;
	//vkGetPhysicalDeviceFeatures(device, &dev_features);

	QueueFamilyIndices indices = findQueueFamilies(device);

	return dev_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		indices.isComplete();
}

QueueFamilyIndices VulkanProg::findQueueFamilies(VkPhysicalDevice dev)
{
	QueueFamilyIndices indices;

	uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);

	std::vector<VkQueueFamilyProperties> families(count);
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, families.data());

	int i = 0;
	for (const auto& family : families) {
		if (family.queueCount > 0 && family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphics_family = i;

		VkBool32 support_presentation = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, m_surface, &support_presentation);

		if (family.queueCount > 0 && support_presentation)
			indices.present_family = i;

		if (indices.isComplete())
			break;

		i++;
	}

	return indices;
}