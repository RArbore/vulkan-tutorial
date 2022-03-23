#include <iostream>
#include <cstring>
#include <vector>
#include <array>
#include <set>

#include <chrono>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

extern "C" char _binary_build_shaders_vert_spv_start;
extern "C" char _binary_build_shaders_vert_spv_end;

extern "C" char _binary_build_shaders_frag_spv_start;
extern "C" char _binary_build_shaders_frag_spv_end;

__attribute__((always_inline))
inline unsigned long long micro_sec() {
    return static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
}


struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class Graphics {
public:
    Graphics();
    ~Graphics();

    bool should_close();
    void render_tick();

    bool frame_buffer_resized = false;
private:
    GLFWwindow *window;
    VkInstance instance;

    VkPhysicalDevice physical_device;
    VkDevice device;

    uint32_t graphics_family_index, present_family_index;
    uint32_t queue_family_indices[2];

    VkQueue graphics_queue, present_queue;

    VkSurfaceKHR surface;
    VkExtent2D swap_extent;
    uint32_t image_count;
    VkSurfaceFormatKHR surface_format;

    VkSwapchainKHR swap_chain;
    std::vector<VkImageView> swap_chain_image_views;
    std::vector<VkImage> swap_chain_images;
    std::vector<VkFramebuffer> swap_chain_framebuffers;

    VkAttachmentDescription color_attachment {};
    VkAttachmentReference color_attachment_reference {};
    VkSubpassDescription subpass {};
    VkSubpassDependency dependency {};
    VkRenderPass render_pass;

    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;
    VkViewport viewport {};
    VkRect2D scissor {};
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;

    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;

    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;
    VkBuffer uniform_buffers;
    VkDeviceMemory uniform_buffers_memory;

    VkDescriptorPool descriptor_pool;
    std::vector<VkDescriptorSet> descriptor_sets;

    VkClearValue clear_color;
    VkPipelineStageFlags wait_stages[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
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
    void create_descriptor_set_layout();
    void create_graphics_pipeline();
    void create_framebuffers();
    void create_command_pool();
    void create_vertex_buffers();
    void create_index_buffers();
    void create_uniform_buffers();
    void create_descriptor_pool();
    void create_descriptor_sets();
    void create_command_buffers();
    void create_sync_objects();
    void update_uniform_buffers(uint32_t current_image);

    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &buffer_memory);
    void copy_buffer(VkBuffer dst_buffer, VkBuffer src_buffer, VkDeviceSize size);
    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);
    void cleanup_swap_chain();
    void recreate_swap_chain();
};
