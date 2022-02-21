#include "VulkanRenderer.h"

const std::vector<const char*> VulkanRenderer::validationLayers{
	"VK_LAYER_KHRONOS_validation"
};


int VulkanRenderer::init(GLFWwindow* windowP) {

	window = windowP;
	try {
		createInstance();
		setupDebugMessenger();
		getPhysicalDevice();
	}
	catch (const std::runtime_error& e) {
		printf("ERROR: %s\n", e.what());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}


void VulkanRenderer::createInstance() {

	/* Information about the application */
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;					// Type of app info
	appInfo.pApplicationName = "Vulkan App";							// Name of the app
	appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);		// Version of the app
	appInfo.pEngineName = "No Engine";									// Custom engine name
	appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);			// Custom engine version
	appInfo.apiVersion = VK_API_VERSION_1_3;							// Vulkan version 

	/* Create Info */
	VkInstanceCreateInfo createInfo{};
	//createInfo.pNext			// Extended information
	//createInfo.flags			// Flags with bitfield
	createInfo.pApplicationInfo = &appInfo;

	/* Setup extensions instance */
	std::vector<const char*> instanceExtensions = getRequiredExtensions();
	
	/* Check extensions */
	if (!checkInstanceExtensionSupport(instanceExtensions)) {
		throw std::runtime_error("VkInstance does not support required extensions");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	/* Validation layers */
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("Validation Layers requested but not available!");
	}
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}

	/* Finally Create Instance */
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
	// Second argument is to choose where to allocate memory

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a Vulkan instance");
	}
}


bool VulkanRenderer::checkInstanceExtensionSupport(const std::vector<const char*>& checkExtensions) {

	/* How many extensions vulkan supports */
	uint32_t extensionCount = 0;
	/* Get the number of extensions */
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	/* Create the vector */
	std::vector<VkExtensionProperties> extensions(extensionCount);
	/* Populate the vector */
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
	// Getting the number of elements then populating the vector is somewhat common in Vulkan //

	/* Check for extensions */
	for (const auto& checkExtensions : checkExtensions) {
		bool hasExtension = false;
		for (const auto& extension : extensions) {
			if (strcmp(checkExtensions, extension.extensionName) == 0) {
				hasExtension = true;
				break;
			}
		}
		if (!hasExtension) {
			return false;
		}
	}
	return true;
}


void VulkanRenderer::clean() {
	vkDestroyDevice(mainDevice.logicalDevice, nullptr);
	vkDestroyInstance(instance, nullptr);					// Second argument is a custom de-allocator
}


VkResult VulkanRenderer::createDebugUtilsMessengerEXT(VkInstance instance, 
													const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
													const VkAllocationCallbacks* pAllocator, 
													VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}


void VulkanRenderer::getPhysicalDevice() {
	
	/* Get the number of devices then populate the physical device vector */
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	/* If no devices area available */
	if (deviceCount == 0) {
		throw std::runtime_error("Can't find any GPU that supports vulkan");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	/* Get valid device */
	for (const auto& device : devices) {
		if (checkDeviceSuitable(device)) {
			mainDevice.physicalDevice = device;
			break;
		}
	}
}


bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device) {

	/* Information about the device itself */
	VkPhysicalDeviceProperties deviceProperties{};
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	/* Information about what the device can do */
	VkPhysicalDeviceFeatures deviceFeatures{};
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	/* For now we do nothing */

	QueueFamilyIndices indices = getQueueFamilies(device);
	return indices.isValid();
}


QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device) {

	QueueFamilyIndices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	/* Loop through families and check for at least one queue */
	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		/* Check there is a graphics queue */
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}
		if (indices.isValid()) {
			break;
		}
		++i;
	}
	return indices;
}


void VulkanRenderer::createLogicalDevice() {

	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

	/* Queues that the logical device needs to create */
	/* Only one queue for now */
	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
	queueCreateInfo.queueCount = 1;

	float priority = 1.0f;
	/* Vulkan needs to know how to handle queues. It uses priorities: 1 is the highest priority */
	queueCreateInfo.pQueuePriorities = &priority;

	/* Logical Device Creation */
	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	/* Queues Info */
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;

	/* Extensions Info */
	deviceCreateInfo.enabledExtensionCount = 0;				// Device extensions, different from instance extensions
	deviceCreateInfo.ppEnabledExtensionNames = nullptr;

	/* Features */
	VkPhysicalDeviceFeatures deviceFeatures{};				// For now, no device features
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	/* Create the logical device for the given physical device */
	VkResult result = vkCreateDevice(mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Could not create the logical device.");
	}

	/* Ensure access to the queues */
	vkGetDeviceQueue(mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
}


bool VulkanRenderer::checkValidationLayerSupport() {

	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	/* Check if all validation layers exists in the available layers */
	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound) {
			return false;
		}
	}
	return true;
}


std::vector<const char*> VulkanRenderer::getRequiredExtensions() {

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	return extensions;
}


void VulkanRenderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {

	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
								 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}


void VulkanRenderer::setupDebugMessenger() {

	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	if (createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("Failed to set up debug messenger.");
	}
}