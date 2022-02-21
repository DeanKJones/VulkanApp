#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>

#include "VulkanUtilities.h"

class VulkanRenderer
{ 
public:

#ifndef NDEBUG
	static const bool enableValidationLayers = false;
#else 
	static const bool enableValidationLayers = true;
#endif 
	static const std::vector<const char*> validationLayers;


	VulkanRenderer() = default;
	~VulkanRenderer() = default;

	int init(GLFWwindow* windowP);
	void clean();

	struct {
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} mainDevice;

private:
	GLFWwindow* window;
	VkInstance instance;
	VkQueue graphicsQueue;
	VkDebugUtilsMessengerEXT debugMessenger;

	void createInstance();
	bool checkInstanceExtensionSupport(const std::vector<const char*>& checkExtensions);
	bool checkValidationLayerSupport();

	void setupDebugMessenger();
	VkResult createDebugUtilsMessengerEXT(VkInstance instance,
										  const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
										  const VkAllocationCallbacks* pAllocator,
										  VkDebugUtilsMessengerEXT* pDebugMessenger);

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		
	void getPhysicalDevice();
	bool checkDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);

	void createLogicalDevice();
	std::vector<const char*> getRequiredExtensions();
};


