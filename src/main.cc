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
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif

int main() {
    glfwInit();
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    auto window = glfwCreateWindow(800, 600, "vulkan-tutorial", nullptr, nullptr);

    if (enable_validation_layers) {
	uint32_t validation_layer_count = 0;
	vkEnumerateInstanceLayerProperties(&validation_layer_count, nullptr);
	VkLayerProperties *available_validation_layers = new VkLayerProperties[validation_layer_count];
	vkEnumerateInstanceLayerProperties(&validation_layer_count, available_validation_layers);
	for (const char* layer_name : validation_layers) {
	    std::size_t i = 0;
	    for (; i < validation_layer_count; ++i) {
		auto layer_props = available_validation_layers[i];
		if (!strcmp(layer_name, layer_props.layerName)) {
		    break;
		}
	    }

	    if (i >= validation_layer_count) {
		VK_ASSERT(VK_ERROR_VALIDATION_FAILED_EXT)
	    }
	}
    }

    VkApplicationInfo app_info;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "vulkan-tutorial";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Custom";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    uint32_t glfw_ext_count = 0;
    const char** glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
    VkInstanceCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = glfw_ext_count;
    create_info.ppEnabledExtensionNames = glfw_exts;
    create_info.enabledLayerCount = 0;

    VkInstance instance;
    VK_ASSERT(vkCreateInstance(&create_info, nullptr, &instance));

    while (!glfwWindowShouldClose(window)) {
	glfwPollEvents();
    }

    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
