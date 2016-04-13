// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include "../include/API/vkapi.h"

command_list_storage_t create_command_storage(device_t dev)
{
	// TODO
	return std::make_shared<vulkan_wrapper::command_pool>(dev->object, 0, 0);
}

command_list_t create_command_list(device_t dev, command_list_storage_t storage)
{
	return std::make_shared<vulkan_wrapper::command_buffer>(dev->object, storage->object, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

buffer_t create_buffer(device_t dev, size_t size)
{
	// TODO: Usage
	return std::make_shared<vulkan_wrapper::buffer>(dev->object, size, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void* map_buffer(device_t dev, buffer_t buffer);
void unmap_buffer(device_t dev, buffer_t buffer);
//image_t create_image(device_t dev, DXGI_FORMAT format, uint32_t width, uint32_t height, uint16_t mipmap, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initial_state, D3D12_CLEAR_VALUE *clear_value);
descriptor_storage_t create_descriptor_storage(device_t dev, uint32_t num_descriptors)
{
	// TODO
	std::vector<VkDescriptorPoolSize> descriptor_pool_size;
	return std::make_shared<vulkan_wrapper::descriptor_pool>(dev->object, 0, 8, descriptor_pool_size);
}

void create_constant_buffer_view(device_t dev, descriptor_storage_t storage, uint32_t index, buffer_t buffer, uint32_t buffer_size);
descriptor_storage_t create_sampler_heap(device_t dev, uint32_t num_descriptors);
void create_sampler(device_t dev, descriptor_storage_t storage, uint32_t index, SAMPLER_TYPE sampler_type);
void create_image_view(device_t dev, descriptor_storage_t storage, uint32_t index, image_t img);
void start_command_list_recording(device_t dev, command_list_t command_list, command_list_storage_t storage);

void make_command_list_executable(command_list_t command_list)
{
	vkEndCommandBuffer(command_list->object);
}

void wait_for_command_queue_idle(device_t dev, command_queue_t command_queue)
{
	vkQueueWaitIdle(command_queue->object);
}

void present(device_t dev, swap_chain_t chain);
void set_pipeline_barrier(device_t dev, command_list_t command_list, image_t resource, RESOURCE_USAGE before, RESOURCE_USAGE after, uint32_t subresource);
void clear_color(device_t dev, command_list_t command_list, framebuffer_t framebuffer, const std::array<float, 4> &color);


void clear_depth_stencil(device_t dev, command_list_t command_list, framebuffer_t framebuffer, depth_stencil_aspect aspect, float depth, uint8_t stencil);
void set_viewport(command_list_t command_list, float x, float width, float y, float height, float min_depth, float max_depth);
void set_scissor(command_list_t command_list, uint32_t left, uint32_t right, uint32_t top, uint32_t bottom);

namespace
{
	VkIndexType get_index_type(irr::video::E_INDEX_TYPE type)
	{
		switch (type)
		{
		case irr::video::E_INDEX_TYPE::EIT_16BIT: return VK_INDEX_TYPE_UINT16;
		case irr::video::E_INDEX_TYPE::EIT_32BIT: return VK_INDEX_TYPE_UINT32;
		}
		throw;
	}
};

void bind_index_buffer(command_list_t command_list, buffer_t buffer, uint64_t offset, uint32_t size, irr::video::E_INDEX_TYPE type)
{
	vkCmdBindIndexBuffer(command_list->object, buffer->object, offset, get_index_type(type));
}

void bind_vertex_buffers(command_list_t commandlist, uint32_t first_bind, const std::vector<std::tuple<buffer_t, uint64_t, uint32_t, uint32_t> > &buffer_offset_stride_size)
{
	std::vector<VkBuffer> pbuffers(buffer_offset_stride_size.size());
	std::vector<VkDeviceSize> poffsets(buffer_offset_stride_size.size());

	size_t idx = 0;
	for (const auto &infos : buffer_offset_stride_size)
	{
		buffer_t buffer;
		uint64_t offset;
		std::tie(buffer, offset, std::ignore, std::ignore) = infos;
		pbuffers[idx] = buffer->object;
		poffsets[idx] = offset;
		idx++;
	}
	vkCmdBindVertexBuffers(commandlist->object, first_bind, static_cast<uint32_t>(buffer_offset_stride_size.size()), pbuffers.data(), poffsets.data());
}

void submit_executable_command_list(command_queue_t command_queue, command_list_t command_list)
{
	VkSubmitInfo info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	info.commandBufferCount = 1;
	info.pCommandBuffers = &(command_list->object);
	vkQueueSubmit(command_queue->object, 1, &info, VK_NULL_HANDLE);
}

void draw_indexed(command_list_t command_list, uint32_t index_count, uint32_t instance_count, uint32_t base_index, int32_t base_vertex, uint32_t base_instance)
{
	vkCmdDrawIndexed(command_list->object, index_count, instance_count, base_index, base_vertex, base_instance);
}
