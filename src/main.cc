#include <iostream>
#include <cstring>
#include <vector>

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
    VkInstanceCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = glfw_ext_count;
    create_info.ppEnabledExtensionNames = glfw_exts;
    create_info.enabledLayerCount = 0;

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

	create_info.enabledLayerCount = validation_layer_count;
	create_info.ppEnabledLayerNames = validation_layers.data();
    }

    VkInstance instance;
    VK_ASSERT(vkCreateInstance(&create_info, nullptr, &instance));

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    uint32_t physical_device_count = 0;
    vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
    if (!physical_device_count) throw std::runtime_error("Vulkan failure");
    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data());
    for (const auto& device: physical_devices) {
	if ([](VkPhysicalDevice check_device) {
	    VkPhysicalDeviceProperties device_properties;
	    vkGetPhysicalDeviceProperties(check_device, &device_properties);
	    VkPhysicalDeviceFeatures device_features;
	    vkGetPhysicalDeviceFeatures(check_device, &device_features);
	    if (enable_debug) std::cout << device_properties.deviceName << std::endl;
	    return true;
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
    uint32_t graphics_queue = 0;
    for (; graphics_queue < queue_families.size(); ++graphics_queue) {
	const auto& queue_family = queue_families[graphics_queue];
	if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) break;
    }
    if (graphics_queue >= queue_families.size()) throw std::runtime_error("Vulkan failure");

    while (!glfwWindowShouldClose(window)) {
	glfwPollEvents();
    }

    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
