// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once
#include <vulkan\vulkan.h>
#include <array>
#include <gsl/gsl>

#define CHECK_VKRESULT(cmd) { VkResult res = (cmd); if (res != VK_SUCCESS) throw; }

namespace structures
{
	constexpr VkAttachmentDescription attachment_description(
		VkFormat format,
		VkAttachmentLoadOp load_op,
		VkAttachmentStoreOp store_op,
		VkImageLayout initial_layout,
		VkImageLayout final_layout,
		VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT,
		VkAttachmentLoadOp stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VkAttachmentStoreOp stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VkAttachmentDescriptionFlags flag = 0)
	{
		return{ flag, format, sample_count, load_op, store_op, stencil_load_op, stencil_store_op, initial_layout, final_layout };
	}
}
