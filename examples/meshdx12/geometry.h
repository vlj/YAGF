// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <tuple>
#include <array>
#include <unordered_map>

#ifdef D3D12
#include <API/d3dapi.h>
#else
#include <API/vkapi.h>
#endif

struct object
{
	std::vector<std::tuple<size_t, size_t, size_t> > meshOffset;
	std::vector<buffer_t> upload_buffers;

	buffer_t vertex_pos;
	buffer_t vertex_uv0;
	buffer_t vertex_normal;
	buffer_t index_buffer;
	size_t total_index_cnt;
	std::vector<std::tuple<buffer_t, uint64_t, uint32_t, uint32_t> > vertex_buffers_info;

	std::vector<uint32_t> texture_mapping;
	std::vector<image_t> Textures;

	buffer_t object_matrix;
#ifndef D3D12
	std::vector<VkDescriptorSet> texture_descriptor_set;
	std::vector<std::shared_ptr<vulkan_wrapper::image_view> > Textures_views;
	std::vector<VkDescriptorSet> mesh_descriptor_set;
	VkDescriptorSet object_descriptor_set;
#endif
	object(device_t dev, const aiScene*, command_list_t upload_cmd_list, descriptor_storage_t heap, vulkan_wrapper::pipeline_descriptor_set* object_set, vulkan_wrapper::pipeline_descriptor_set* model_set);

	void fill_draw_command(device_t dev, command_list_t cmd_list, pipeline_layout_t object_sig);
	void update_constant_buffers(device_t dev);
};
