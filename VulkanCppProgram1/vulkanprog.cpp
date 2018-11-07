#include "vulkanprog.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <glm/glm.hpp>

#include <array>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>

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


struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription binding_info = {};
		binding_info.binding = 0;
		binding_info.stride = sizeof(Vertex);
		binding_info.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return binding_info;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attrib_info = {};

		attrib_info[0].binding = 0;
		attrib_info[0].location = 0;
		attrib_info[0].format = VK_FORMAT_R32G32_SFLOAT;
		attrib_info[0].offset = offsetof(Vertex, pos);

		attrib_info[1].binding = 0;
		attrib_info[1].location = 1;
		attrib_info[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attrib_info[1].offset = offsetof(Vertex, color);

		return attrib_info;
	}
};


const std::vector<Vertex> g_vertices = {
	{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
	{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};


static std::vector<char> read_shader(const std::string& path)
{
	std::ifstream fp(path, std::ios::ate | std::ios::binary);

	if (!fp.is_open())
		throw std::runtime_error("Failed to open shader file.");

	size_t size = (size_t)fp.tellg();
	std::vector<char> buffer(size);

	fp.seekg(0);
	fp.read(buffer.data(), size);
	fp.close();

	return buffer;
}


static void framebufferResizeCb(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<VulkanProg*>(glfwGetWindowUserPointer(window));
	app->setFramebufferResized(true);
}


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
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();
	createVertexBuffer();
	createCommandBuffers();
	createSyncObjects();
}

void VulkanProg::initWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	m_window = glfwCreateWindow(WIDTH, HEIGHT, "Basic triangle with Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, framebufferResizeCb);
}

void VulkanProg::mainLoop()
{
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
		drawFrame();
	}

	vkDeviceWaitIdle(m_logical_device);
}

void VulkanProg::cleanup()
{
	cleanupSwapChain();

	vkDestroyBuffer(m_logical_device, m_vertex_buffer, nullptr);
	vkFreeMemory(m_logical_device, m_vertex_buffer_memory, nullptr);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkDestroySemaphore(m_logical_device, m_image_available_semaphores[i], nullptr);
		vkDestroySemaphore(m_logical_device, m_render_finished_semaphores[i], nullptr);
		vkDestroyFence(m_logical_device, m_inflight_fences[i], nullptr);
	}

	vkDestroyCommandPool(m_logical_device, m_command_pool, nullptr);
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

void VulkanProg::createRenderPass()
{
	VkAttachmentDescription color_attach = {};
	color_attach.format = m_swapchain_format;
	color_attach.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attach.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attach_ref = {};
	color_attach_ref.attachment = 0;
	color_attach_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass_info = {};
	subpass_info.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_info.colorAttachmentCount = 1;
	subpass_info.pColorAttachments = &color_attach_ref;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &color_attach;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass_info;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;

	if (vkCreateRenderPass(m_logical_device, &render_pass_info, nullptr, &m_renderpass) != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass.");
}

void VulkanProg::createGraphicsPipeline()
{
	std::vector<char> vert_code;
	try {
		vert_code = read_shader("shaders/vert.spv");
	}
	catch (std::runtime_error e) {
		std::cerr << e.what() << std::endl;
		return;
	}

	std::vector<char> frag_code;
	try {
		frag_code = read_shader("shaders/frag.spv");
	}
	catch (std::runtime_error e) {
		std::cerr << e.what() << std::endl;
		return;
	}

	VkShaderModule vert_shader;
	try {
		vert_shader = createShaderModule(vert_code);
	}
	catch (std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		vert_code.clear();
		return;
	}

	VkShaderModule frag_shader;
	try {
		frag_shader = createShaderModule(frag_code);
	}
	catch (std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		frag_code.clear();
		vkDestroyShaderModule(m_logical_device, vert_shader, nullptr);
		return;
	}

	VkPipelineShaderStageCreateInfo vertex_stage_info = {};
	vertex_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertex_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertex_stage_info.module = vert_shader;
	vertex_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_stage_info = {};
	frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_stage_info.module = frag_shader;
	frag_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vertex_stage_info, frag_stage_info };

	// Vertex Input stage
	auto binding_desc = Vertex::getBindingDescription();
	auto attrib_desc = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &binding_desc;
	vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrib_desc.size());
	vertex_input_info.pVertexAttributeDescriptions = attrib_desc.data();

	// Vertex Assembly stage
	VkPipelineInputAssemblyStateCreateInfo vertex_assembly_info = {};
	vertex_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	vertex_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	vertex_assembly_info.primitiveRestartEnable = VK_FALSE;

	// Viewport definition
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_swapchain_extent.width;
	viewport.height = (float)m_swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = m_swapchain_extent;

	// Viewport state definition
	VkPipelineViewportStateCreateInfo viewport_info = {};
	viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_info.viewportCount = 1;
	viewport_info.pViewports = &viewport;
	viewport_info.scissorCount = 1;
	viewport_info.pScissors = &scissor;

	// Rasterizer state
	VkPipelineRasterizationStateCreateInfo raster_info = {};
	raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster_info.depthClampEnable = VK_FALSE;
	raster_info.rasterizerDiscardEnable = VK_FALSE;
	raster_info.polygonMode = VK_POLYGON_MODE_FILL;
	raster_info.lineWidth = 1.0f;
	raster_info.cullMode = VK_CULL_MODE_BACK_BIT;
	raster_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
	raster_info.depthBiasEnable = VK_FALSE;
	raster_info.depthBiasConstantFactor = 0.0f;
	raster_info.depthBiasClamp = 0.0f;
	raster_info.depthBiasSlopeFactor = 0.0f;

	// Multisampling state definition
	VkPipelineMultisampleStateCreateInfo msample_info = {};
	msample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	msample_info.sampleShadingEnable = VK_FALSE;
	msample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	msample_info.minSampleShading = 1.0f;
	msample_info.pSampleMask = nullptr;
	msample_info.alphaToCoverageEnable = VK_FALSE;
	msample_info.alphaToOneEnable = VK_FALSE;

	// Depth and stencil tests state definition
	// No depth or stencil tests

	// Color blending state definition
	VkPipelineColorBlendAttachmentState color_blend_attach = {};
	color_blend_attach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attach.blendEnable = VK_FALSE;
	color_blend_attach.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attach.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attach.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attach.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blend_info = {};
	color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_info.logicOpEnable = VK_FALSE;
	color_blend_info.logicOp = VK_LOGIC_OP_COPY;
	color_blend_info.attachmentCount = 1;
	color_blend_info.pAttachments = &color_blend_attach;
	color_blend_info.blendConstants[0] = 0.0f;
	color_blend_info.blendConstants[1] = 0.0f;
	color_blend_info.blendConstants[2] = 0.0f;
	color_blend_info.blendConstants[3] = 0.0f;

	// Pipeline layout creation
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 0;
	pipeline_layout_info.pSetLayouts = nullptr;
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(m_logical_device, &pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create pipeline layout.");

	// Actual pipeline creation
	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = shader_stages;
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &vertex_assembly_info;
	pipeline_info.pViewportState = &viewport_info;
	pipeline_info.pRasterizationState = &raster_info;
	pipeline_info.pMultisampleState = &msample_info;
	pipeline_info.pDepthStencilState = nullptr;
	pipeline_info.pColorBlendState = &color_blend_info;
	pipeline_info.pDynamicState = nullptr;
	pipeline_info.layout = m_pipeline_layout;
	pipeline_info.renderPass = m_renderpass;
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_info.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(m_logical_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_graphics_pipeline) != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics pipeline.");

	vkDestroyShaderModule(m_logical_device, vert_shader, nullptr);
	vkDestroyShaderModule(m_logical_device, frag_shader, nullptr);
}

void VulkanProg::createFramebuffers()
{
	m_swapchain_framebuffers.resize(m_swapchain_image_views.size());

	for (size_t i = 0; i < m_swapchain_image_views.size(); ++i) {
		VkImageView attach[] = {
			m_swapchain_image_views[i]
		};

		VkFramebufferCreateInfo fb_info = {};
		fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fb_info.renderPass = m_renderpass;
		fb_info.attachmentCount = 1;
		fb_info.pAttachments = attach;
		fb_info.width = m_swapchain_extent.width;
		fb_info.height= m_swapchain_extent.height;
		fb_info.layers = 1;

		if (vkCreateFramebuffer(m_logical_device, &fb_info, nullptr, &m_swapchain_framebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create framebuffer.");
	}
}

void VulkanProg::createCommandPool()
{
	QueueFamilyIndices indices = findQueueFamilies(m_device);

	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = indices.graphics_family.value();
	pool_info.flags = 0;

	if (vkCreateCommandPool(m_logical_device, &pool_info, nullptr, &m_command_pool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create command pool.");
}

void VulkanProg::createCommandBuffers()
{
	m_command_buffers.resize(m_swapchain_framebuffers.size());

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = m_command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = static_cast<uint32_t>(m_command_buffers.size());

	if (vkAllocateCommandBuffers(m_logical_device, &alloc_info, m_command_buffers.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate command buffers.");

	for (size_t i = 0; i < m_command_buffers.size(); ++i) {
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		begin_info.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(m_command_buffers[i], &begin_info) != VK_SUCCESS)
			throw std::runtime_error("Failed to begin recording command buffer.");

		VkRenderPassBeginInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = m_renderpass;
		render_pass_info.framebuffer = m_swapchain_framebuffers[i];
		render_pass_info.renderArea.offset = { 0, 0 };
		render_pass_info.renderArea.extent = m_swapchain_extent;

		VkClearValue clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
		render_pass_info.clearValueCount = 1;
		render_pass_info.pClearValues = &clear_color;

		vkCmdBeginRenderPass(m_command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(m_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline);

		VkBuffer vertex_buffers[] = { m_vertex_buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_command_buffers[i], 0, 1, vertex_buffers, offsets);

		vkCmdDraw(m_command_buffers[i], static_cast<uint32_t>(g_vertices.size()), 1, 0, 0);
		vkCmdEndRenderPass(m_command_buffers[i]);

		if (vkEndCommandBuffer(m_command_buffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to record command buffer.");
	}
}

void VulkanProg::createSyncObjects()
{
	m_image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_inflight_fences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		if (vkCreateSemaphore(m_logical_device, &semaphore_info, nullptr, &m_image_available_semaphores[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create image_available semaphore.");
		if (vkCreateSemaphore(m_logical_device, &semaphore_info, nullptr, &m_render_finished_semaphores[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create render_finished semaphore.");
		if (vkCreateFence(m_logical_device, &fence_info, nullptr, &m_inflight_fences[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create inflight fence.");
	}
}

void VulkanProg::drawFrame()
{
	vkWaitForFences(m_logical_device, 1, &m_inflight_fences[m_current_frame], VK_TRUE, std::numeric_limits<uint64_t>::max());

	uint32_t image_idx;
	VkResult result = vkAcquireNextImageKHR(m_logical_device, m_swapchain, std::numeric_limits<uint64_t>::max(), m_image_available_semaphores[m_current_frame], VK_NULL_HANDLE, &image_idx);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		rebuildSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("Failed to aquire swap chain image");

	VkSemaphore wait_semaphores[] = { m_image_available_semaphores[m_current_frame] };
	VkSemaphore signal_semaphores[] = { m_render_finished_semaphores[m_current_frame] };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_command_buffers[image_idx];
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphores;

	vkResetFences(m_logical_device, 1, &m_inflight_fences[m_current_frame]);
	if (vkQueueSubmit(m_graphics_queue, 1, &submit_info, m_inflight_fences[m_current_frame]) != VK_SUCCESS)
		throw std::runtime_error("Failed to submit draw command buffer");

	VkSwapchainKHR swap_chains[] = { m_swapchain };

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphores;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swap_chains;
	present_info.pImageIndices = &image_idx;
	present_info.pResults = nullptr;

	result = vkQueuePresentKHR(m_presentation_queue, &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebuffer_resized) {
		m_framebuffer_resized = false;
		rebuildSwapChain();
	}
	else if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to present swap chain image.");

	m_current_frame = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanProg::createVertexBuffer()
{
	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = sizeof(g_vertices[0]) * g_vertices.size();
	buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(m_logical_device, &buffer_info, nullptr, &m_vertex_buffer) != VK_SUCCESS)
		throw std::runtime_error("Failed to create vertex buffer.");

	VkMemoryRequirements mem_requirements = {};
	vkGetBufferMemoryRequirements(m_logical_device, m_vertex_buffer, &mem_requirements);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = findMemoryType(mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (vkAllocateMemory(m_logical_device, &alloc_info, nullptr, &m_vertex_buffer_memory) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate vertex buffer memory.");

	vkBindBufferMemory(m_logical_device, m_vertex_buffer, m_vertex_buffer_memory, 0);

	void* data;
	vkMapMemory(m_logical_device, m_vertex_buffer_memory, 0, buffer_info.size, 0, &data);
	memcpy(data, g_vertices.data(), (size_t)buffer_info.size);
	vkUnmapMemory(m_logical_device, m_vertex_buffer_memory);
}

void VulkanProg::cleanupSwapChain()
{
	for (auto fb : m_swapchain_framebuffers)
		vkDestroyFramebuffer(m_logical_device, fb, nullptr);

	vkFreeCommandBuffers(m_logical_device, m_command_pool, static_cast<uint32_t>(m_command_buffers.size()), m_command_buffers.data());

	vkDestroyPipeline(m_logical_device, m_graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(m_logical_device, m_pipeline_layout, nullptr);
	vkDestroyRenderPass(m_logical_device, m_renderpass, nullptr);

	for (auto iv : m_swapchain_image_views)
		vkDestroyImageView(m_logical_device, iv, nullptr);

	vkDestroySwapchainKHR(m_logical_device, m_swapchain, nullptr);
}

void VulkanProg::rebuildSwapChain()
{
	int width = 0;
	int height = 0;
	while (!width || !height) {
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_logical_device);

	cleanupSwapChain();

	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandBuffers();
}

VkShaderModule VulkanProg::createShaderModule(const std::vector<char>& bytecode)
{
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = static_cast<uint32_t>(bytecode.size());
	create_info.pCode = reinterpret_cast<const uint32_t*>(bytecode.data());

	VkShaderModule shader;
	if (vkCreateShaderModule(m_logical_device, &create_info, nullptr, &shader) != VK_SUCCESS)
		throw std::runtime_error("Failed to create shader module.");

	return shader;
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
		int width, height;
		glfwGetFramebufferSize(m_window, &width, &height);
		VkExtent2D actual_extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
		actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		return actual_extent;
	}
}

uint32_t VulkanProg::findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(m_device, &mem_props);

	for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i)
		if ((type_filter & (1 << i)) && (mem_props.memoryTypes[i].propertyFlags & properties) == properties)
			return i;

	throw std::runtime_error("Failed to find suitable memory type.");
}