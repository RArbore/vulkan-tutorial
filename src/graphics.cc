#include "graphics.h"

static constexpr int WIDTH = 800;
static constexpr int HEIGHT = 600;
static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validation_layers = {
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char*> device_extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription get_binding_description() {
	VkVertexInputBindingDescription desc;
	desc.binding = 0;
	desc.stride = sizeof(Vertex);
	desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return desc;
    }

    static std::array<VkVertexInputAttributeDescription, 2> get_attribute_descriptions() {
	std::array<VkVertexInputAttributeDescription, 2> descs;
	descs[0].binding = 0;
	descs[0].location = 0;
	descs[0].format = VK_FORMAT_R32G32_SFLOAT;
	descs[0].offset = offsetof(Vertex, pos);
	descs[1].binding = 0;
	descs[1].location = 1;
	descs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	descs[1].offset = offsetof(Vertex, color);
	return descs;
    }
};

const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{0.0f+0.6f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f+0.6f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f+0.6f, 0.5f}, {0.0f, 0.0f, 1.0f}},
};

#ifdef NDEBUG
static constexpr bool enable_debug = false;
#else
static constexpr bool enable_debug = true;
#endif

Graphics::Graphics() {
    glfw_init();
    create_instance();
    create_surface();
    create_physical_device();
    create_logical_device();
    create_swap_chain();
    create_image_views();
    create_render_pass();
    create_graphics_pipeline();
    create_framebuffers();
    create_command_pool();
    create_vertex_buffers();
    create_command_buffers();
    create_sync_objects();
}

Graphics::~Graphics() {
    vkDeviceWaitIdle(device);
    for (auto fn : in_flight_fences)
	vkDestroyFence(device, fn, nullptr);
    for (auto sm : render_finished_semaphores)
	vkDestroySemaphore(device, sm, nullptr);
    for (auto sm : image_available_semaphores)
	vkDestroySemaphore(device, sm, nullptr);
    vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(command_buffers.size()), command_buffers.data());
    vkDestroyBuffer(device, vertex_buffer, nullptr);
    vkFreeMemory(device, vertex_buffer_memory, nullptr);
    vkDestroyCommandPool(device, command_pool, nullptr);
    for (auto fb : swap_chain_framebuffers)
	vkDestroyFramebuffer(device, fb, nullptr);
    vkDestroyPipeline(device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);
    vkDestroyShaderModule(device, vert_shader_module, nullptr);
    vkDestroyShaderModule(device, frag_shader_module, nullptr);
    for (auto swap_chain_image_view : swap_chain_image_views)
	vkDestroyImageView(device, swap_chain_image_view, nullptr);
    vkDestroySwapchainKHR(device, swap_chain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Graphics::should_close() {
    return glfwWindowShouldClose(window);
}

void Graphics::render_tick() {
    glfwPollEvents();
    
    vkWaitForFences(device, 1, &in_flight_fences.at(current_frame), VK_TRUE, UINT64_MAX);
    
    uint32_t image_index;
    vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX, image_available_semaphores.at(current_frame), VK_NULL_HANDLE, &image_index);
    
    if (images_in_flight.at(current_frame) != VK_NULL_HANDLE)
	vkWaitForFences(device, 1, &images_in_flight.at(current_frame), VK_TRUE, UINT64_MAX);
    images_in_flight.at(current_frame) = in_flight_fences.at(current_frame);
    vkResetFences(device, 1, &in_flight_fences.at(current_frame));
    
    submit_info.pCommandBuffers = &command_buffers.at(image_index);
    submit_info.pWaitSemaphores = &image_available_semaphores.at(current_frame);
    submit_info.pSignalSemaphores = &render_finished_semaphores.at(current_frame);
    vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences.at(current_frame));
    
    present_info.pImageIndices = &image_index;
    present_info.pWaitSemaphores = &render_finished_semaphores.at(current_frame);
    vkQueuePresentKHR(present_queue, &present_info);
    
    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    if (frame_buffer_resized) {
	frame_buffer_resized = false;
	recreate_swap_chain();
	return;
    }
}

static void framebuffer_resize_callback(GLFWwindow *window, [[maybe_unused]] int width, [[maybe_unused]] int height) {
    auto graphics = reinterpret_cast<Graphics*>(glfwGetWindowUserPointer(window));
    graphics->frame_buffer_resized = true;
}

void Graphics::glfw_init() {
    glfwInit();
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "vulkan-tutorial", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
}

void Graphics::create_instance() {
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

    VK_ASSERT(vkCreateInstance(&instance_create_info, nullptr, &instance));
}

void Graphics::create_surface() {
    VK_ASSERT(glfwCreateWindowSurface(instance, window, nullptr, &surface));
}

void Graphics::create_physical_device() {
    physical_device = VK_NULL_HANDLE;
    uint32_t physical_device_count = 0;
    vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
    if (!physical_device_count) throw std::runtime_error("Vulkan failure");
    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data());
    for (const auto& poss_device : physical_devices) {
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
	}(poss_device)) {
	    physical_device = poss_device;
	    break;
	}
    }
    if (physical_device == VK_NULL_HANDLE) throw std::runtime_error("Vulkan failure");
}

void Graphics::create_logical_device() {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());
    graphics_family_index = 0;
    present_family_index = static_cast<uint32_t>(queue_families.size());
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

    VK_ASSERT(vkCreateDevice(physical_device, &device_create_info, nullptr, &device));

    vkGetDeviceQueue(device, graphics_family_index, 0, &graphics_queue);
    vkGetDeviceQueue(device, present_family_index, 0, &present_queue);
}

void Graphics::create_swap_chain() {
    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
    if (surface_capabilities.currentExtent.width != UINT32_MAX) {
	swap_extent = surface_capabilities.currentExtent;
    }
    else {
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	swap_extent.width = static_cast<uint32_t>(width);
	swap_extent.height = static_cast<uint32_t>(height);
    }

    if (swap_extent.width < surface_capabilities.minImageExtent.width || swap_extent.height < surface_capabilities.minImageExtent.height) {
	swap_extent.width = surface_capabilities.minImageExtent.width;
	swap_extent.height = surface_capabilities.minImageExtent.height;
    }
    if (swap_extent.width > surface_capabilities.maxImageExtent.width || swap_extent.height > surface_capabilities.maxImageExtent.height) {
	swap_extent.width = surface_capabilities.maxImageExtent.width;
	swap_extent.height = surface_capabilities.maxImageExtent.height;
    }
    
    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats.data());
    surface_format = formats.at(0);
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

    image_count = surface_capabilities.minImageCount + 1;
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
    queue_family_indices[0] = graphics_family_index;
    queue_family_indices[1] = present_family_index;
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

    VK_ASSERT(vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swap_chain));
}

void Graphics::create_image_views() {
    vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr);
    swap_chain_images.resize(image_count);
    vkGetSwapchainImagesKHR(device, swap_chain, &image_count, swap_chain_images.data());

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
}

void Graphics::create_render_pass() {
    color_attachment.format = surface_format.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_reference;

    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_create_info {};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &color_attachment;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &dependency;
    
    VK_ASSERT(vkCreateRenderPass(device, &render_pass_create_info, nullptr, &render_pass));
}

void Graphics::create_graphics_pipeline() {
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

    VK_ASSERT(vkCreateShaderModule(device, &vert_shader_module_create_info, nullptr, &vert_shader_module));
    
    VkShaderModuleCreateInfo frag_shader_module_create_info {};
    frag_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    frag_shader_module_create_info.codeSize = frag_size;
    frag_shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(frag_spv.data());

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

    auto binding_description = Vertex::get_binding_description();
    auto attribute_descriptions = Vertex::get_attribute_descriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input_create_info {};
    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_create_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_create_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info {};
    input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swap_extent.width);
    viewport.height = static_cast<float>(swap_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    scissor.offset = {0, 0};
    scissor.extent = swap_extent;

    VkPipelineViewportStateCreateInfo viewport_state_create_info {};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports = &viewport;
    viewport_state_create_info.scissorCount = 1;
    viewport_state_create_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_state_create_info {};
    rasterizer_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_state_create_info.depthClampEnable = VK_FALSE;
    rasterizer_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_state_create_info.lineWidth = 1.0f;
    rasterizer_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer_state_create_info.depthBiasEnable = VK_FALSE;
    rasterizer_state_create_info.depthBiasConstantFactor = 0.0f;
    rasterizer_state_create_info.depthBiasClamp = 0.0f;
    rasterizer_state_create_info.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state_create_info {};
    multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_create_info.minSampleShading = 1.0f;
    multisample_state_create_info.pSampleMask = nullptr;
    multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_create_info.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info {};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.logicOpEnable = VK_FALSE;
    color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &color_blend_attachment;
    color_blend_state_create_info.blendConstants[0] = 0.0f;
    color_blend_state_create_info.blendConstants[1] = 0.0f;
    color_blend_state_create_info.blendConstants[2] = 0.0f;
    color_blend_state_create_info.blendConstants[3] = 0.0f;
    
    VkPipelineLayoutCreateInfo pipeline_layout_create_info {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = 0;
    pipeline_layout_create_info.pSetLayouts = nullptr;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = nullptr;
    
    VK_ASSERT(vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &pipeline_layout));

    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info {};
    graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_create_info.stageCount = 2;
    graphics_pipeline_create_info.pStages = shader_stages_create_info;
    graphics_pipeline_create_info.pVertexInputState = &vertex_input_create_info;
    graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
    graphics_pipeline_create_info.pViewportState = &viewport_state_create_info;
    graphics_pipeline_create_info.pRasterizationState = &rasterizer_state_create_info;
    graphics_pipeline_create_info.pMultisampleState = &multisample_state_create_info;
    graphics_pipeline_create_info.pDepthStencilState = nullptr;
    graphics_pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
    graphics_pipeline_create_info.pDynamicState = nullptr;
    graphics_pipeline_create_info.layout = pipeline_layout;
    graphics_pipeline_create_info.renderPass = render_pass;
    graphics_pipeline_create_info.subpass = 0;
    graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    graphics_pipeline_create_info.basePipelineIndex = -1;

    VK_ASSERT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, nullptr, &graphics_pipeline));
}

void Graphics::create_framebuffers() {
    swap_chain_framebuffers.resize(swap_chain_image_views.size());
    for (std::size_t i = 0; i < swap_chain_framebuffers.size(); ++i) {
	VkFramebufferCreateInfo framebuffer_create_info {};
	framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_create_info.renderPass = render_pass;
	framebuffer_create_info.attachmentCount = 1;
	framebuffer_create_info.pAttachments = &swap_chain_image_views.at(i);
	framebuffer_create_info.width = swap_extent.width;
	framebuffer_create_info.height = swap_extent.height;
	framebuffer_create_info.layers = 1;

	VK_ASSERT(vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &swap_chain_framebuffers.at(i)));
    }
}

void Graphics::create_command_pool() {
    VkCommandPoolCreateInfo command_pool_create_info {};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.queueFamilyIndex = graphics_family_index;
    command_pool_create_info.flags = 0;

    VK_ASSERT(vkCreateCommandPool(device, &command_pool_create_info, nullptr, &command_pool));
}

void Graphics::create_vertex_buffers() {
    std::size_t size = sizeof(vertices[0]) * vertices.size();
    create_buffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertex_buffer, vertex_buffer_memory);

    void *data;
    vkMapMemory(device, vertex_buffer_memory, 0, size, 0, &data);
    memcpy(data, vertices.data(), size);
    vkUnmapMemory(device, vertex_buffer_memory);
}

void Graphics::create_command_buffers() {
    VkCommandBufferAllocateInfo command_buffer_allocate_info {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = static_cast<uint32_t>(swap_chain_framebuffers.size());
    
    command_buffers.resize(swap_chain_framebuffers.size());
    VK_ASSERT(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, command_buffers.data()));

    clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    for (std::size_t i = 0; i < command_buffers.size(); ++i) {
	VkCommandBufferBeginInfo command_buffer_begin_info {};
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.flags = 0;
	command_buffer_begin_info.pInheritanceInfo = nullptr;
	VK_ASSERT(vkBeginCommandBuffer(command_buffers.at(i), &command_buffer_begin_info));

	VkRenderPassBeginInfo render_pass_begin_info {};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.renderPass = render_pass;
	render_pass_begin_info.framebuffer = swap_chain_framebuffers.at(i);
	render_pass_begin_info.renderArea.offset = {0, 0};
	render_pass_begin_info.renderArea.extent = swap_extent;
	render_pass_begin_info.clearValueCount = 1;
	render_pass_begin_info.pClearValues = &clear_color;
	vkCmdBeginRenderPass(command_buffers.at(i), &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(command_buffers.at(i), VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(command_buffers.at(i), 0, 1, &vertex_buffer, &offset);
	vkCmdDraw(command_buffers.at(i), static_cast<uint32_t>(vertices.size()), 1, 0, 0);

	vkCmdEndRenderPass(command_buffers.at(i));
	VK_ASSERT(vkEndCommandBuffer(command_buffers.at(i)));
    }
}

void Graphics::create_sync_objects() {
    VkSemaphoreCreateInfo semaphore_create_info {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_create_info {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
    images_in_flight.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
    for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
	VK_ASSERT(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &image_available_semaphores.at(i)));
	VK_ASSERT(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &render_finished_semaphores.at(i)));
	VK_ASSERT(vkCreateFence(device, &fence_create_info, nullptr, &in_flight_fences.at(i)));
    }

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.signalSemaphoreCount = 1;

    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swap_chain;
    present_info.pResults = nullptr;
}

void Graphics::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &buffer_memory) {
    VkBufferCreateInfo buffer_create_info {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_ASSERT(vkCreateBuffer(device, &buffer_create_info, nullptr, &buffer));

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(device, buffer, &mem_reqs);

    VkMemoryAllocateInfo memory_allocate_info {};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = mem_reqs.size;
    memory_allocate_info.memoryTypeIndex = find_memory_type(mem_reqs.memoryTypeBits, properties);

    VK_ASSERT(vkAllocateMemory(device, &memory_allocate_info, nullptr, &buffer_memory));
    vkBindBufferMemory(device, buffer, buffer_memory, 0);
}

uint32_t Graphics::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);

    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
	if ((type_filter & (1 << i)) && (mem_props.memoryTypes[i].propertyFlags & properties)) return i;
    }
    throw std::runtime_error("Failed to find memory");
}

void Graphics::recreate_swap_chain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
	glfwGetFramebufferSize(window, &width, &height);
	glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    for (auto fb : swap_chain_framebuffers)
	vkDestroyFramebuffer(device, fb, nullptr);
    vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(command_buffers.size()), command_buffers.data());
    vkDestroyPipeline(device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);
    vkDestroyShaderModule(device, vert_shader_module, nullptr);
    vkDestroyShaderModule(device, frag_shader_module, nullptr);
    for (auto swap_chain_image_view : swap_chain_image_views)
	vkDestroyImageView(device, swap_chain_image_view, nullptr);
    swap_chain_image_views.clear();
    vkDestroySwapchainKHR(device, swap_chain, nullptr);

    create_swap_chain();
    create_image_views();
    create_render_pass();
    create_graphics_pipeline();
    create_framebuffers();
    create_command_buffers();
}
