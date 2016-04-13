// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include "../include/API/vkapi.h"

command_list_storage_t create_command_storage(device_t dev)
{
	command_list_storage_t result;
	return result;
}
command_list_t create_command_list(device_t dev, command_list_storage_t storage);
buffer_t create_buffer(device_t dev, size_t size);
void* map_buffer(device_t dev, buffer_t buffer);
void unmap_buffer(device_t dev, buffer_t buffer);
//image_t create_image(device_t dev, DXGI_FORMAT format, uint32_t width, uint32_t height, uint16_t mipmap, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initial_state, D3D12_CLEAR_VALUE *clear_value);
descriptor_storage_t create_descriptor_storage(device_t dev, uint32_t num_descriptors);
void create_constant_buffer_view(device_t dev, descriptor_storage_t storage, uint32_t index, buffer_t buffer, uint32_t buffer_size);
descriptor_storage_t create_sampler_heap(device_t dev, uint32_t num_descriptors);
void create_sampler(device_t dev, descriptor_storage_t storage, uint32_t index, SAMPLER_TYPE sampler_type);
void create_image_view(device_t dev, descriptor_storage_t storage, uint32_t index, image_t img);
void start_command_list_recording(device_t dev, command_list_t command_list, command_list_storage_t storage);
void make_command_list_executable(device_t dev, command_list_t command_list);
void wait_for_command_queue_idle(device_t dev, command_queue_t command_queue);
void present(device_t dev, swap_chain_t chain);
void set_pipeline_barrier(device_t dev, command_list_t command_list, image_t resource, RESOURCE_USAGE before, RESOURCE_USAGE after, uint32_t subresource);
void clear_color(device_t dev, command_list_t command_list, framebuffer_t framebuffer, const std::array<float, 4> &color);


void clear_depth_stencil(device_t dev, command_list_t command_list, framebuffer_t framebuffer, depth_stencil_aspect aspect, float depth, uint8_t stencil);
void set_viewport(command_list_t command_list, float x, float width, float y, float height, float min_depth, float max_depth);
void set_scissor(command_list_t command_list, uint32_t left, uint32_t right, uint32_t top, uint32_t bottom);


void bind_index_buffer(command_list_t command_list, buffer_t buffer, uint64_t offset, uint32_t size, index_buffer_type type);
void bind_vertex_buffers(command_list_t commandlist, uint32_t first_bind, const std::vector<std::tuple<buffer_t, uint64_t, uint32_t, uint32_t> > &buffer_offset_stride_size);
void submit_executable_command_list(command_queue_t command_queue, command_list_t command_list);
void draw_indexed(command_list_t command_list, uint32_t index_count, uint32_t instance_count, uint32_t base_index, int32_t base_vertex, uint32_t base_instance);
