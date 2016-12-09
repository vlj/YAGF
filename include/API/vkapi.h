// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan\vulkan.hpp>
#include <memory>
#include <vector>
#include "..\Core\SColor.h"
#include <fstream>
#include <API\GfxApi.h>
#include <glfw/glfw3.h>


#define CHECK_VKRESULT(cmd) { VkResult res = (cmd); if (res != VK_SUCCESS) throw; }

#include "..\VKAPI\vulkan_helpers.h"

struct vk_command_list_storage_t : command_list_storage_t
{
	virtual std::unique_ptr<command_list_t> create_command_list() override;
	virtual void reset_command_list_storage() override;
	vk_command_list_storage_t(vk::Device _dev, vk::CommandPool _object)
		: dev(_dev), object(_object)
	{}

	virtual ~vk_command_list_storage_t() override
	{
		dev.destroyCommandPool(object);
	}
private:
	vk::Device dev;
	vk::CommandPool object;
};

struct vk_command_list_t : command_list_t
{
	virtual void bind_graphic_descriptor(uint32_t bindpoint, const allocated_descriptor_set & descriptor_set, pipeline_layout_t& sig) override;
	virtual void bind_compute_descriptor(uint32_t bindpoint, const allocated_descriptor_set & descriptor_set, pipeline_layout_t& sig) override;
	virtual void copy_buffer_to_image_subresource(image_t & destination_image, uint32_t destination_subresource, buffer_t & source, uint64_t offset_in_buffer, uint32_t width, uint32_t height, uint32_t row_pitch, irr::video::ECOLOR_FORMAT format) override;
	virtual void set_pipeline_barrier(image_t & resource, RESOURCE_USAGE before, RESOURCE_USAGE after, uint32_t subresource, irr::video::E_ASPECT) override;
	virtual void set_uav_flush(image_t & resource) override;
	virtual void set_viewport(float x, float width, float y, float height, float min_depth, float max_depth) override;
	virtual void set_scissor(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom) override;
	virtual void set_graphic_pipeline(pipeline_state_t& pipeline) override;
	virtual void set_graphic_pipeline_layout(pipeline_layout_t & sig) override;
	virtual void set_compute_pipeline(compute_pipeline_state_t & pipeline) override;
	virtual void set_compute_pipeline_layout(pipeline_layout_t & sig) override;
	virtual void set_descriptor_storage_referenced(descriptor_storage_t & main_heap, descriptor_storage_t * sampler_heap = nullptr) override;
	virtual void bind_index_buffer(buffer_t & buffer, uint64_t offset, uint32_t size, irr::video::E_INDEX_TYPE type) override;
	virtual void bind_vertex_buffers(uint32_t first_bind, const std::vector<std::tuple<buffer_t&, uint64_t, uint32_t, uint32_t>>& buffer_offset_stride_size) override;
	virtual void draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t base_index, int32_t base_vertex, uint32_t base_instance) override;
	virtual void draw_non_indexed(uint32_t vertex_count, uint32_t instance_count, int32_t base_vertex, uint32_t base_instance) override;
	virtual void dispatch(uint32_t x, uint32_t y, uint32_t z) override;
	virtual void copy_buffer(buffer_t & src, uint64_t src_offset, buffer_t & dst, uint64_t dst_offset, uint64_t size) override;
	virtual void next_subpass() override;
	virtual void end_renderpass() override;
	virtual void make_command_list_executable() override;
	virtual void start_command_list_recording(command_list_storage_t& storage) override;

	vk::Device dev;
	vk::CommandBuffer object;
	vk_command_list_t(vk::Device _dev, vk::CommandBuffer _object) : dev(_dev), object(_object)
	{}

	virtual void begin_renderpass(render_pass_t& rp, framebuffer_t &fbo,
		gsl::span<clear_value_t> clear_values,
		uint32_t width, uint32_t height) override;
};

struct vk_device_t : device_t
{
	vk_device_t(vk::Device _dev) : object(_dev)
	{}

	uint32_t queue_family_index;
	vk::PhysicalDeviceMemoryProperties mem_properties;
	virtual std::unique_ptr<command_list_storage_t> create_command_storage() override;
	virtual std::unique_ptr<buffer_t> create_buffer(size_t size, irr::video::E_MEMORY_POOL memory_pool, uint32_t flags) override;
	virtual std::unique_ptr<buffer_view_t> create_buffer_view(buffer_t &, irr::video::ECOLOR_FORMAT, uint64_t offset, uint32_t size) override;
	virtual void set_constant_buffer_view(const allocated_descriptor_set & descriptor_set, uint32_t offset_in_set, uint32_t binding_location, buffer_t & buffer, uint32_t buffer_size, uint64_t offset_in_buffer = 0) override;
	virtual void set_uniform_texel_buffer_view(const allocated_descriptor_set & descriptor_set, uint32_t offset_in_set, uint32_t binding_location, buffer_view_t & buffer_view) override;
	virtual void set_uav_buffer_view(const allocated_descriptor_set & descriptor_set, uint32_t offset_in_set, uint32_t binding_location, buffer_t & buffer, uint64_t offset, uint32_t size) override;
	virtual std::unique_ptr<image_t> create_image(irr::video::ECOLOR_FORMAT format, uint32_t width, uint32_t height, uint16_t mipmap, uint32_t layers, uint32_t flags, clear_value_t * clear_value) override;
	virtual std::unique_ptr<image_view_t> create_image_view(image_t & img, irr::video::ECOLOR_FORMAT fmt, uint16_t base_mipmap, uint16_t mipmap_count, uint16_t base_layer, uint16_t layer_count, irr::video::E_TEXTURE_TYPE texture_type, irr::video::E_ASPECT aspect = irr::video::E_ASPECT::EA_COLOR) override;
	virtual void set_image_view(const allocated_descriptor_set & descriptor_set, uint32_t offset, uint32_t binding_location, image_view_t & img_view) override;
	virtual void set_input_attachment(const allocated_descriptor_set & descriptor_set, uint32_t offset, uint32_t binding_location, image_view_t & img_view) override;
	virtual void set_uav_image_view(const allocated_descriptor_set & descriptor_set, uint32_t offset, uint32_t binding_location, image_view_t & img_view) override;
	virtual void set_sampler(const allocated_descriptor_set & descriptor_set, uint32_t offset, uint32_t binding_location, sampler_t & sampler) override;
	virtual std::unique_ptr<sampler_t> create_sampler(SAMPLER_TYPE sampler_type) override;
	virtual std::unique_ptr<descriptor_storage_t> create_descriptor_storage(uint32_t num_sets, const std::vector<std::tuple<RESOURCE_VIEW, uint32_t>>& num_descriptors) override;
	virtual std::unique_ptr<framebuffer_t> create_frame_buffer(gsl::span<const image_view_t*> render_targets, uint32_t width, uint32_t height, render_pass_t * render_pass) override;
	virtual std::unique_ptr<framebuffer_t> create_frame_buffer(gsl::span<const image_view_t*> render_targets, const image_view_t& depth_stencil_texture, uint32_t width, uint32_t height, render_pass_t * render_pass) override;
	virtual std::unique_ptr<descriptor_set_layout> get_object_descriptor_set(const descriptor_set_ &ds) override;

	vk::Device object;
	vk::Instance instance;

	virtual std::unique_ptr<pipeline_state_t> create_graphic_pso(const graphic_pipeline_state_description &) override;
	virtual std::unique_ptr<compute_pipeline_state_t> create_compute_pso(const compute_pipeline_state_description &) override;
	virtual std::unique_ptr<pipeline_layout_t> create_pipeline_layout(gsl::span<const descriptor_set_layout*>) override;

	virtual ~vk_device_t()
	{
		object.destroy();
		instance.destroy();
	}

	// Hérité via device_t
	virtual std::unique_ptr<render_pass_t> create_ibl_sky_pass() override;
	virtual std::unique_ptr<render_pass_t> create_object_sunlight_pass() override;
};

struct vk_command_queue_t : command_queue_t
{
	vk_command_queue_t(vk::Queue _object) : object(_object)
	{}

	virtual void submit_executable_command_list(command_list_t & command_list) override;
	virtual void wait_for_command_queue_idle() override;

	vk::Queue object;
};

struct vk_buffer_t : buffer_t
{
	virtual void * map_buffer() override;
	virtual void unmap_buffer() override;

	vk_buffer_t(vk::Device _dev, vk::Buffer _object, vk::DeviceMemory _memory)
		: object(_object), dev(_dev), memory(_memory)
	{}

	vk::Buffer object;
	vk::DeviceMemory memory;
	vk::Device dev;
};

struct vk_image_t : image_t
{
	vk_image_t(vk::Device _dev, vk::Image _object, vk::DeviceMemory _memory, uint32_t _mip_levels)
		: object(_object), dev(_dev), memory(_memory), mip_levels(_mip_levels)
	{}

	vk::Image object;
	vk::DeviceMemory memory;
	vk::Device dev;
	uint32_t mip_levels;
};

struct vk_descriptor_storage_t : descriptor_storage_t {
	vk_descriptor_storage_t(vk::Device _dev, vk::DescriptorPool _object)
		: object(_object), dev(_dev)
	{}

	vk::DescriptorPool object;
	vk::Device dev;

	virtual std::unique_ptr<allocated_descriptor_set> allocate_descriptor_set_from_cbv_srv_uav_heap(uint32_t starting_index, const std::vector<descriptor_set_layout*> layouts, uint32_t descriptors_count) override;
	virtual std::unique_ptr<allocated_descriptor_set> allocate_descriptor_set_from_sampler_heap(uint32_t starting_index, const std::vector<descriptor_set_layout*> layouts, uint32_t descriptors_count) override;

	virtual ~vk_descriptor_storage_t()
	{
		dev.destroyDescriptorPool(object);
	}
};

struct vk_pipeline_state_t : pipeline_state_t {
	vk::Pipeline object;
	vk::Device dev;

	vk_pipeline_state_t(vk::Device _dev, vk::Pipeline pso) : dev(_dev), object(pso)
	{}

	virtual ~vk_pipeline_state_t()
	{
		dev.destroyPipeline(object);
	}
};

struct vk_compute_pipeline_state_t : compute_pipeline_state_t {
	vk::Pipeline object;
	vk::Device dev;

	vk_compute_pipeline_state_t(vk::Device _dev, vk::Pipeline pso) : dev(_dev), object(pso)
	{}

	virtual ~vk_compute_pipeline_state_t()
	{
		dev.destroyPipeline(object);
	}
};

struct vk_pipeline_layout_t : pipeline_layout_t
{
	vk::PipelineLayout object;
	vk::Device dev;
	~vk_pipeline_layout_t() {
		dev.destroyPipelineLayout(object);
	}
};

struct vk_swap_chain_t : swap_chain_t
{
	vk_swap_chain_t(vk::Device _dev, vk::SwapchainKHR _object) : dev(_dev), object(_object)
	{}

	vk::SwapchainKHR object;
	vk::Device dev;

	virtual uint32_t get_next_backbuffer_id() override;
	virtual std::vector<std::unique_ptr<image_t>> get_image_view_from_swap_chain() override;
	virtual void present(command_queue_t & cmdqueue, uint32_t backbuffer_index) override;
};

struct vk_render_pass_t : render_pass_t {
	vk::RenderPass object;
	vk::Device dev;

	vk_render_pass_t(vk::Device _dev, vk::RenderPass _object)
		: dev(_dev), object(_object)
	{}

	~vk_render_pass_t()
	{
		dev.destroyRenderPass(object);
	}
};

struct vk_allocated_descriptor_set : allocated_descriptor_set {
	vk_allocated_descriptor_set(vk::DescriptorSet _object) : object(_object)
	{}

	vk::DescriptorSet object;
};

struct vk_descriptor_set_layout : descriptor_set_layout {
	vk::DescriptorSetLayout object;
	vk::Device dev;

	vk_descriptor_set_layout(vk::Device _dev, vk::DescriptorSetLayout _o) : dev(_dev), object(_o)
	{}

	virtual ~vk_descriptor_set_layout()
	{
		dev.destroyDescriptorSetLayout(object);
	}
};

struct vk_image_view_t : image_view_t {
	virtual ~vk_image_view_t() {
		dev.destroyImageView(object);
	}

	vk_image_view_t(vk::Device _dev, vk::ImageView _object) : dev(_dev), object(_object) {}

	vk::Device dev;
	vk::ImageView object;
};

struct vk_sampler_t : sampler_t {
	vk_sampler_t(vk::Device _dev, vk::Sampler _object) : dev(_dev), object(_object)
	{}

	vk::Sampler object;
	vk::Device dev;
};

struct vk_buffer_view_t : buffer_view_t {
	vk::BufferView object;
	vk::Device dev;

	vk_buffer_view_t(vk::Device _dev, vk::BufferView bf) : dev(_dev), object(bf)
	{}

	virtual ~vk_buffer_view_t()
	{
		dev.destroyBufferView(object);
	}
};

struct vk_framebuffer : framebuffer_t
{
	vk::Framebuffer object;
	vk::Device dev;

	vk_framebuffer(vk::Device _dev, vk::RenderPass render_pass, gsl::span<const vk::ImageView> render_targets, uint32_t width, uint32_t height, uint32_t layers);

	virtual ~vk_framebuffer()
	{
		dev.destroyFramebuffer(object);
	}
};


#include "../VKAPI/pipeline_helpers.h"
#include "../VKAPI/pipeline_layout_helpers.h"
#include "../VKAPI/renderpass_helpers.h"

std::tuple<std::unique_ptr<device_t>, std::unique_ptr<swap_chain_t>, std::unique_ptr<command_queue_t>, uint32_t, uint32_t, irr::video::ECOLOR_FORMAT> create_device_swapchain_and_graphic_presentable_queue(GLFWwindow *window);
