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

extern "C" char _binary_build_shaders_vert_spv_start;
extern "C" char _binary_build_shaders_vert_spv_end;

extern "C" char _binary_build_shaders_frag_spv_start;
extern "C" char _binary_build_shaders_frag_spv_end;

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
	VkDeviceQueueCreateInfo queue_create_info {};
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queueFamilyIndex = family;
	queue_create_info.queueCount = 1;
	queue_create_info.pQueuePriorities = &queue_priority;
	queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features {};
    VkDeviceCreateInfo device_create_info {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    device_create_info.ppEnabledExtensionNames = device_extensions.data();

    VkDevice device;
    VK_ASSERT(vkCreateDevice(physical_device, &device_create_info, nullptr, &device));

    VkQueue graphics_queue, present_queue;
    vkGetDeviceQueue(device, graphics_family_index, 0, &graphics_queue);
    vkGetDeviceQueue(device, present_family_index, 0, &present_queue);

    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
    VkExtent2D swap_extent = surface_capabilities.currentExtent;

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats.data());
    VkSurfaceFormatKHR surface_format = formats.at(0);
    for (const auto& available_format : formats) {
	if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
	    surface_format = available_format;
	    break;
	}
    }

    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);
    std::vector<VkPresentModeKHR> present_modes(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes.data());
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& available_present_mode : present_modes) {
	if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
	    present_mode = available_present_mode;
	    break;
	}
    }

    uint32_t image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount) image_count = surface_capabilities.maxImageCount;
    VkSwapchainCreateInfoKHR swapchain_create_info {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = surface;
    swapchain_create_info.minImageCount = image_count;
    swapchain_create_info.imageFormat = surface_format.format;
    swapchain_create_info.imageColorSpace = surface_format.colorSpace;
    swapchain_create_info.imageExtent = swap_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    uint32_t queue_family_indices[] = {graphics_family_index, present_family_index};
    if (graphics_family_index != present_family_index) {
	swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
	swapchain_create_info.queueFamilyIndexCount = 2;
	swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else {
	swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_create_info.queueFamilyIndexCount = 0;
	swapchain_create_info.pQueueFamilyIndices = nullptr;
    }
    swapchain_create_info.preTransform = surface_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swap_chain;
    VK_ASSERT(vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swap_chain));

    vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr);
    std::vector<VkImage> swap_chain_images(image_count);
    vkGetSwapchainImagesKHR(device, swap_chain, &image_count, swap_chain_images.data());

    std::vector<VkImageView> swap_chain_image_views;
    for (const auto& swap_chain_image : swap_chain_images) {
	VkImageViewCreateInfo image_view_create_info {};
	image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.image = swap_chain_image;
	image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.format = surface_format.format;
	image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_view_create_info.subresourceRange.baseMipLevel = 0;
	image_view_create_info.subresourceRange.levelCount = 1;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.layerCount = 1;
	swap_chain_image_views.emplace_back();
	VK_ASSERT(vkCreateImageView(device, &image_view_create_info, nullptr, &swap_chain_image_views.back()));
    }

    std::size_t vert_size = static_cast<std::size_t>(&_binary_build_shaders_vert_spv_end - &_binary_build_shaders_vert_spv_start);
    std::size_t frag_size = static_cast<std::size_t>(&_binary_build_shaders_frag_spv_end - &_binary_build_shaders_frag_spv_start);
    std::vector<char> vert_spv(vert_size);
    std::vector<char> frag_spv(frag_size);
    memcpy(vert_spv.data(), &_binary_build_shaders_vert_spv_start, vert_size);
    memcpy(frag_spv.data(), &_binary_build_shaders_frag_spv_start, frag_size);

    VkShaderModuleCreateInfo vert_shader_module_create_info {};
    vert_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vert_shader_module_create_info.codeSize = vert_size;
    vert_shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(vert_spv.data());

    VkShaderModule vert_shader_module;
    VK_ASSERT(vkCreateShaderModule(device, &vert_shader_module_create_info, nullptr, &vert_shader_module));
    
    VkShaderModuleCreateInfo frag_shader_module_create_info {};
    frag_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    frag_shader_module_create_info.codeSize = frag_size;
    frag_shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(frag_spv.data());

    VkShaderModule frag_shader_module;
    VK_ASSERT(vkCreateShaderModule(device, &frag_shader_module_create_info, nullptr, &frag_shader_module));

    VkPipelineShaderStageCreateInfo vert_shader_stage_create_info {};
    vert_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_create_info.module = vert_shader_module;
    vert_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_create_info {};
    frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_create_info.module = frag_shader_module;
    frag_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages_create_info[] = {vert_shader_stage_create_info, frag_shader_stage_create_info};
    
    while (!glfwWindowShouldClose(window)) {
	glfwPollEvents();
    }

    vkDestroyShaderModule(device, vert_shader_module, nullptr);
    vkDestroyShaderModule(device, frag_shader_module, nullptr);
    for (auto swap_chain_image_view : swap_chain_image_views) {
	vkDestroyImageView(device, swap_chain_image_view, nullptr);
    }
    vkDestroySwapchainKHR(device, swap_chain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
