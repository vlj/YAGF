// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#include <vector>
#include <tuple>
#include <array>
#include "..\Core\SColor.h"

namespace irr
{
	namespace video
	{
		enum class E_INDEX_TYPE
		{
			EIT_16BIT,
			EIT_32BIT
		};
	}
}

enum class RESOURCE_USAGE
{
	PRESENT,
	COPY_DEST,
	COPY_SRC,
	RENDER_TARGET,
	READ_GENERIC,
	DEPTH_WRITE,
};

enum image_flags
{
	usage_transfer_src = 0x1,
	usage_transfer_dst = 0x2,
	usage_sampled = 0x4,
	usage_render_target = 0x8,
	usage_depth_stencil = 0x10,
	usage_input_attachment = 0x20,
	usage_cube = 0x40,
};

enum class RESOURCE_VIEW
{
	CONSTANTS_BUFFER,
	SHADER_RESOURCE,
	SAMPLER,
	UAV,
};

enum class SAMPLER_TYPE
{
	NEAREST,
	BILINEAR,
	TRILINEAR,
	ANISOTROPIC,
};

struct MipLevelData
{
	size_t Offset;
	size_t Width;
	size_t Height;
	size_t RowPitch;
};

command_list_storage_t create_command_storage(device_t dev);
command_list_t create_command_list(device_t dev, command_list_storage_t storage);
buffer_t create_buffer(device_t dev, size_t size);
image_t create_image(device_t dev, irr::video::ECOLOR_FORMAT format, uint32_t width, uint32_t height, uint16_t mipmap, uint32_t flags, RESOURCE_USAGE initial_state, clear_value_structure_t *clear_value);
descriptor_storage_t create_descriptor_storage(device_t dev, uint32_t num_descriptors);
descriptor_storage_t create_sampler_heap(device_t dev, uint32_t num_descriptors);
framebuffer_t create_frame_buffer(device_t dev, std::vector<std::tuple<image_t, irr::video::ECOLOR_FORMAT>> render_targets);
framebuffer_t create_frame_buffer(device_t dev, std::vector<std::tuple<image_t, irr::video::ECOLOR_FORMAT>> render_targets, std::tuple<image_t, irr::video::ECOLOR_FORMAT> depth_stencil_texture);
void create_constant_buffer_view(device_t dev, descriptor_storage_t storage, uint32_t index, buffer_t buffer, uint32_t buffer_size);
void create_sampler(device_t dev, descriptor_storage_t storage, uint32_t index, SAMPLER_TYPE sampler_type);
void create_image_view(device_t dev, descriptor_storage_t storage, uint32_t index, image_t img);

void* map_buffer(device_t dev, buffer_t buffer);
void unmap_buffer(device_t dev, buffer_t buffer);

void start_command_list_recording(device_t dev, command_list_t command_list, command_list_storage_t storage);
void make_command_list_executable(command_list_t command_list);
void wait_for_command_queue_idle(device_t dev, command_queue_t command_queue);
void present(device_t dev, swap_chain_t chain);
void set_pipeline_barrier(device_t dev, command_list_t command_list, image_t resource, RESOURCE_USAGE before, RESOURCE_USAGE after, uint32_t subresource);
void clear_color(device_t dev, command_list_t command_list, framebuffer_t framebuffer, const std::array<float, 4> &color);

enum class depth_stencil_aspect
{
	depth_only,
	stencil_only,
	depth_and_stencil
};
void clear_depth_stencil(device_t dev, command_list_t command_list, framebuffer_t framebuffer, depth_stencil_aspect aspect, float depth, uint8_t stencil);
void set_viewport(command_list_t command_list, float x, float width, float y, float height, float min_depth, float max_depth);
void set_scissor(command_list_t command_list, uint32_t left, uint32_t right, uint32_t top, uint32_t bottom);

void bind_index_buffer(command_list_t command_list, buffer_t buffer, uint64_t offset, uint32_t size, irr::video::E_INDEX_TYPE type);
void bind_vertex_buffers(command_list_t commandlist, uint32_t first_bind, const std::vector<std::tuple<buffer_t, uint64_t, uint32_t, uint32_t> > &buffer_offset_stride_size);
void submit_executable_command_list(command_queue_t command_queue, command_list_t command_list);
void draw_indexed(command_list_t command_list, uint32_t index_count, uint32_t instance_count, uint32_t base_index, int32_t base_vertex, uint32_t base_instance);
uint32_t get_next_backbuffer_id(device_t dev, swap_chain_t chain);
