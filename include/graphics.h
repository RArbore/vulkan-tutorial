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

class Graphics {
public:
    Graphics();
    ~Graphics();

    bool should_close();
    void render_tick();
private:
    GLFWwindow *window;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    uint32_t graphics_family_index, present_family_index;
    VkDevice device;
    VkQueue graphics_queue, present_queue;
    VkExtent2D swap_extent;
    uint32_t image_count;
    uint32_t queue_family_indices[2];
    VkSurfaceFormatKHR surface_format;
    VkSwapchainKHR swap_chain;
    std::vector<VkImageView> swap_chain_image_views;
    std::vector<VkImage> swap_chain_images;
    VkAttachmentDescription color_attachment {};
    VkAttachmentReference color_attachment_reference {};
    VkSubpassDescription subpass {};
    VkSubpassDependency dependency {};
    VkRenderPass render_pass;
    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;
    VkViewport viewport {};
    VkRect2D scissor {};
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;
    std::vector<VkFramebuffer> swap_chain_framebuffers;
    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;
    VkClearValue clear_color;
    VkPipelineStageFlags wait_stages[2] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submit_info {};
    VkPresentInfoKHR present_info {};
    std::vector<VkSemaphore> image_available_semaphores, render_finished_semaphores;
    std::vector<VkFence> in_flight_fences, images_in_flight;
    std::size_t current_frame = 0;

    void glfw_init();
    void create_instance();
    void create_surface();
    void create_physical_device();
    void create_logical_device();
    void create_swap_chain();
    void create_image_views();
    void create_render_pass();
    void create_graphics_pipeline();
    void create_framebuffers();
    void create_command_pool();
    void create_command_buffers();
    void create_sync_objects();
};
