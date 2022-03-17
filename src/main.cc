#include <iostream>
#include <cstring>
#include <vector>
#include <set>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define VK_ASSERT(res) {				\
	if ((res) != VK_SUCCESS) {			\
	    throw std::runtime_error("Vulkan failure");	\
	}						\
    }

const std::vector<const char*> validation_layers = {
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char*> device_extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

#ifdef NDEBUG
static constexpr bool enable_debug = false;
#else
static constexpr bool enable_debug = true;
#endif

int main() {
    glfwInit();
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    auto window = glfwCreateWindow(800, 600, "vulkan-tutorial", nullptr, nullptr);

    VkApplicationInfo app_info {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "vulkan-tutorial";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Custom";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    uint32_t glfw_ext_count = 0;
    const char** glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
    VkInstanceCreateInfo instance_create_info {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &app_info;
    instance_create_info.enabledExtensionCount = glfw_ext_count;
    instance_create_info.ppEnabledExtensionNames = glfw_exts;
    instance_create_info.enabledLayerCount = 0;

    if (enable_debug) {
	uint32_t validation_layer_count = 0;
	vkEnumerateInstanceLayerProperties(&validation_layer_count, nullptr);
	std::vector<VkLayerProperties> available_validation_layers(validation_layer_count);
	vkEnumerateInstanceLayerProperties(&validation_layer_count, available_validation_layers.data());
	for (const char* layer_name : validation_layers) {
	    std::size_t i = 0;
	    for (; i < validation_layer_count; ++i) {
		auto layer_props = available_validation_layers[i];
		if (!strcmp(layer_name, layer_props.layerName)) {
		    break;
		}
	    }
	    if (i >= validation_layer_count) throw std::runtime_error("Vulkan failure");
	}

	instance_create_info.enabledLayerCount = validation_layer_count;
	instance_create_info.ppEnabledLayerNames = validation_layers.data();
    }

    VkInstance instance;
    VK_ASSERT(vkCreateInstance(&instance_create_info, nullptr, &instance));

    VkSurfaceKHR surface;
    VK_ASSERT(glfwCreateWindowSurface(instance, window, nullptr, &surface));

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    uint32_t physical_device_count = 0;
    vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
    if (!physical_device_count) throw std::runtime_error("Vulkan failure");
    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data());
    for (const auto& device : physical_devices) {
	if ([](VkPhysicalDevice check_device) {
	    VkPhysicalDeviceProperties device_properties;
	    vkGetPhysicalDeviceProperties(check_device, &device_properties);
	    VkPhysicalDeviceFeatures device_features;
	    vkGetPhysicalDeviceFeatures(check_device, &device_features);
	    if (enable_debug) std::cout << device_properties.deviceName << std::endl;

	    uint32_t extension_count;
	    vkEnumerateDeviceExtensionProperties(check_device, nullptr, &extension_count, nullptr);
	    std::vector<VkExtensionProperties> available_extensions(extension_count);
	    vkEnumerateDeviceExtensionProperties(check_device, nullptr, &extension_count, available_extensions.data());

	    std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());
	    for (const auto& extension : available_extensions) {
		required_extensions.erase(extension.extensionName);
	    }
	    
	    return required_extensions.empty();
	}(device)) {
	    physical_device = device;
	    break;
	}
    }
    if (physical_device == VK_NULL_HANDLE) throw std::runtime_error("Vulkan failure");

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());
    uint32_t graphics_family_index = 0, present_family_index = static_cast<uint32_t>(queue_families.size());
    for (; graphics_family_index < queue_families.size(); ++graphics_family_index) {
	VkBool32 present_support = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, graphics_family_index, surface, &present_support);
	if (present_support) present_family_index = graphics_family_index;
	const auto& queue_family = queue_families[graphics_family_index];
	if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) break;
    }
    if (graphics_family_index >= queue_families.size()) throw std::runtime_error("Vulkan failure");
    if (present_family_index >= queue_families.size()) throw std::runtime_error("Vulkan failure");
    std::set<uint32_t> unique_queue_families = {graphics_family_index, present_family_index};

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    float queue_priority = 1.0f;
    for (auto family : unique_queue_families) {
	VkDeviceQueueCreateInfo queue_create_info{};
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queueFamilyIndex = family;
	queue_create_info.queueCount = 1;
	queue_create_info.pQueuePriorities = &queue_priority;
	queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features{};
    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    device_create_info.pEnabledFeatures = &device_features;

    VkDevice device;
    VK_ASSERT(vkCreateDevice(physical_device, &device_create_info, nullptr, &device));

    VkQueue graphics_queue, present_queue;
    vkGetDeviceQueue(device, graphics_family_index, 0, &graphics_queue);
    vkGetDeviceQueue(device, present_family_index, 0, &present_queue);

    while (!glfwWindowShouldClose(window)) {
	glfwPollEvents();
    }

    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
