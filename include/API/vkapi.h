// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan\vulkan.h>
#include <vulkan\vk_platform.h>
#include <memory>
#include <vector>
#include "..\Core\SColor.h"
#include <fstream>


#define CHECK_VKRESULT(cmd) { VkResult res = (cmd); if (res != VK_SUCCESS) throw; }

#include "..\VKAPI\vulkan_helpers.h"

using command_list_storage_t = vulkan_wrapper::command_pool;
using command_list_t = vulkan_wrapper::command_buffer;
using device_t = vulkan_wrapper::device;
using command_queue_t = vulkan_wrapper::queue;
using buffer_t = vulkan_wrapper::buffer;
using image_t = vulkan_wrapper::image;
using descriptor_storage_t = vulkan_wrapper::descriptor_pool;
using pipeline_state_t = std::shared_ptr<vulkan_wrapper::pipeline>;
using compute_pipeline_state_t = vulkan_wrapper::compute_pipeline;
using pipeline_layout_t = std::shared_ptr<vulkan_wrapper::pipeline_layout>;
using swap_chain_t = vulkan_wrapper::swapchain;
using render_pass_t = vulkan_wrapper::render_pass;
using clear_value_structure_t = void*;
using allocated_descriptor_set = VkDescriptorSet;
using descriptor_set_layout = vulkan_wrapper::pipeline_descriptor_set;

struct vk_framebuffer
{
	const std::vector<std::shared_ptr<vulkan_wrapper::image_view> > image_views;
	vulkan_wrapper::framebuffer fbo;

	vk_framebuffer(device_t* dev, render_pass_t* render_pass, std::vector<std::tuple<image_t*, irr::video::ECOLOR_FORMAT>> render_targets, uint32_t width, uint32_t height, uint32_t layers);
	vk_framebuffer(device_t* dev, render_pass_t* render_pass, const std::vector<std::tuple<image_t*, irr::video::ECOLOR_FORMAT>> &render_targets, const std::tuple<image_t*, irr::video::ECOLOR_FORMAT> &depth_stencil, uint32_t width, uint32_t height, uint32_t layers);
private:
	std::vector<std::shared_ptr<vulkan_wrapper::image_view> > build_image_views(VkDevice dev, const std::vector<std::tuple<image_t*, irr::video::ECOLOR_FORMAT>> &render_targets, const std::tuple<image_t*, irr::video::ECOLOR_FORMAT> &depth_stencil);
	std::vector<std::shared_ptr<vulkan_wrapper::image_view> > build_image_views(VkDevice dev, const std::vector<std::tuple<image_t*, irr::video::ECOLOR_FORMAT>> &render_targets);
};

using framebuffer_t = std::shared_ptr<vk_framebuffer>;


#include "GfxApi.h"
#include "../VKAPI/pipeline_helpers.h"
#include "../VKAPI/pipeline_layout_helpers.h"
#include "../VKAPI/renderpass_helpers.h"

std::tuple<std::unique_ptr<device_t>, std::unique_ptr<swap_chain_t>, std::unique_ptr<command_queue_t>, size_t, size_t, irr::video::ECOLOR_FORMAT> create_device_swapchain_and_graphic_presentable_queue(HINSTANCE hinstance, HWND window);
std::unique_ptr<vulkan_wrapper::image_view> create_image_view(device_t& dev, image_t& img, VkFormat fmt, VkImageSubresourceRange range, VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D, VkComponentMapping mapping  = structures::component_mapping());
