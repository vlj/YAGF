// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#include <vector>
#include <tuple>

enum class RESOURCE_USAGE
{
  PRESENT,
  COPY_DEST,
  COPY_SRC,
  RENDER_TARGET,
  READ_GENERIC,
  DEPTH_STENCIL,
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
void* map_buffer(device_t dev, buffer_t buffer);
void unmap_buffer(device_t dev, buffer_t buffer);
image_t create_image(device_t dev, DXGI_FORMAT format, uint32_t width, uint32_t height, uint16_t mipmap, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initial_state, D3D12_CLEAR_VALUE *clear_value);
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
