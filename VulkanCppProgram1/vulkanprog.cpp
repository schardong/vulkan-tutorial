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


struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR surface_capabilities;
	std::vector<VkSurfaceFormatKHR> surface_formats;
	std::vector<VkPresentModeKHR> present_modes;
};


bool checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extension_count = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

	std::set<std::string> required_extentions(device_extensions.begin(), device_extensions.end());
	for (const auto& extension : available_extensions)
		required_extentions.erase(extension.extensionName);

	return required_extentions.empty();
}


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
	createSwapChain();
	createImageViews();
	createGraphicsPipeline();
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
	for (auto iv : m_swapchain_image_views)
		vkDestroyImageView(m_logical_device, iv, nullptr);

	vkDestroySwapchainKHR(m_logical_device, m_swapchain, nullptr);
	vkDestroyDevice(m_logical_device, nullptr);

	if (enable_validation_layer)
		destroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);

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
	device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	device_create_info.ppEnabledExtensionNames = device_extensions.data();

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

void VulkanProg::createSwapChain()
{
	SwapChainSupportDetails swap_chain_support = querySwapChainSupport(m_device);

	VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swap_chain_support.surface_formats);
	VkPresentModeKHR present_mode = chooseSwapPresentMode(swap_chain_support.present_modes);
	VkExtent2D extent = chooseSwapExtent(swap_chain_support.surface_capabilities);

	uint32_t img_count = swap_chain_support.surface_capabilities.minImageCount + 1;
	if (swap_chain_support.surface_capabilities.maxImageCount > 0 && img_count > swap_chain_support.surface_capabilities.maxImageCount)
		img_count = swap_chain_support.surface_capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = m_surface;
	create_info.minImageCount = img_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilies(m_device);
	uint32_t queue_family_indices[] = { indices.graphics_family.value(), indices.present_family.value() };

	if (indices.graphics_family != indices.present_family) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	}
	else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = nullptr;
	}

	create_info.preTransform = swap_chain_support.surface_capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_logical_device, &create_info, nullptr, &m_swapchain) != VK_SUCCESS)
		throw std::runtime_error("Failed to create swap chain");

	m_swapchain_format = surface_format.format;
	m_swapchain_extent = extent;

	uint32_t image_count = 0;
	vkGetSwapchainImagesKHR(m_logical_device, m_swapchain, &image_count, nullptr);
	m_swapchain_images.resize(image_count);
	vkGetSwapchainImagesKHR(m_logical_device, m_swapchain, &image_count, m_swapchain_images.data());
}

void VulkanProg::createImageViews()
{
	m_swapchain_image_views.resize(m_swapchain_images.size());

	for (size_t i = 0; i < m_swapchain_images.size(); ++i) {
		VkImageViewCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = m_swapchain_images[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = m_swapchain_format;

		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;
			 
		if (vkCreateImageView(m_logical_device, &create_info, nullptr, &m_swapchain_image_views[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create image view.");
	}
}

void VulkanProg::createGraphicsPipeline()
{

}

bool VulkanProg::isDeviceSuitable(VkPhysicalDevice device)
{
	//VkPhysicalDeviceProperties dev_properties;
	//vkGetPhysicalDeviceProperties(device, &dev_properties);

	//VkPhysicalDeviceFeatures dev_features;
	//vkGetPhysicalDeviceFeatures(device, &dev_features);

	QueueFamilyIndices indices = findQueueFamilies(device);
	bool extensions_supported = checkDeviceExtensionSupport(device);

	bool swap_chain_adequate = false;
	if (extensions_supported) {
		SwapChainSupportDetails swap_chain_support = querySwapChainSupport(device);
		swap_chain_adequate = !swap_chain_support.surface_formats.empty() && !swap_chain_support.present_modes.empty();
	}

	return indices.isComplete() && extensions_supported && swap_chain_adequate;
	//dev_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		
}

QueueFamilyIndices VulkanProg::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

	std::vector<VkQueueFamilyProperties> families(count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

	int i = 0;
	for (const auto& family : families) {
		if (family.queueCount > 0 && family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphics_family = i;

		VkBool32 support_presentation = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &support_presentation);

		if (family.queueCount > 0 && support_presentation)
			indices.present_family = i;

		if (indices.isComplete())
			break;

		i++;
	}

	return indices;
}

SwapChainSupportDetails VulkanProg::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.surface_capabilities);

	uint32_t count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &count, nullptr);
	if (count) {
		details.surface_formats.resize(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &count, details.surface_formats.data());
	}

	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &count, nullptr);
	if (count) {
		details.present_modes.resize(count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &count, details.present_modes.data());
	}

	return details;
}

VkSurfaceFormatKHR VulkanProg::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
	if (available_formats.size() == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED)
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	
	for (const auto& available_format : available_formats)
		if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return available_format;

	return available_formats[0];
}

VkPresentModeKHR VulkanProg::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
	VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& available_present_mode : available_present_modes) {
		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return available_present_mode;
		else if (available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			best_mode = available_present_mode;
	}

	return best_mode;
}

VkExtent2D VulkanProg::chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;
	else {
		VkExtent2D actual_extent = { static_cast<uint32_t>(WIDTH), static_cast<uint32_t>(HEIGHT) };
		actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		return actual_extent;
	}
}
