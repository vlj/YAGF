// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan\vulkan.h>
#include <vulkan\vk_platform.h>

struct subpass_description
{
	const VkPipelineBindPoint pipeline_bind_point;
	const uint32_t input_attachements_count = 0;
	const VkAttachmentReference* input_attachments = nullptr;
	const uint32_t color_attachements_count = 0;
	const VkAttachmentReference* color_attachments = nullptr;
	const uint32_t resolve_attachments_count = 0;
	const VkAttachmentReference* resolve_attachments = nullptr;
	const VkAttachmentReference* depth_stencil = nullptr;
	const uint32_t preserve_attachments_count = 0;
	const uint32_t* preserve_attachments = nullptr;

	static constexpr
	subpass_description generate_subpass_description(VkPipelineBindPoint binding_point)
	{
		return subpass_description(binding_point, 0, nullptr, 0, nullptr, 0, nullptr, nullptr, 0, nullptr);
	}

	template<size_t N>
	constexpr subpass_description set_input_attachments(const VkAttachmentReference(&att)[N]) const
	{
		return subpass_description(pipeline_bind_point, N, att, color_attachements_count, color_attachments, resolve_attachments_count, resolve_attachments, depth_stencil, preserve_attachments_count, preserve_attachments);
	}

	template<size_t N>
	constexpr subpass_description set_color_attachments(const VkAttachmentReference(&att)[N]) const
	{
		return subpass_description(pipeline_bind_point, input_attachements_count, input_attachments, N, att, resolve_attachments_count, resolve_attachments, depth_stencil, preserve_attachments_count, preserve_attachments);
	}

	template<size_t N>
	constexpr subpass_description set_resolve_attachments(const VkAttachmentReference(&att)[N]) const
	{
		return subpass_description(pipeline_bind_point,
			input_attachements_count, input_attachments,
			color_attachements_count, color_attachments,
			N, att,
			depth_stencil, preserve_attachments_count, preserve_attachments);
	}

	constexpr subpass_description set_depth_stencil_attachment(const VkAttachmentReference att) const
	{
		return subpass_description(pipeline_bind_point,
			input_attachements_count, input_attachments,
			color_attachements_count, color_attachments,
			resolve_attachments_count, resolve_attachments,
			&att, preserve_attachments_count, preserve_attachments);
	}

	template<size_t N>
	constexpr subpass_description& set_preserve_attachments(const uint32_t(&att)[N]) const
	{
		return subpass_description(pipeline_bind_point,
			input_attachements_count, input_attachments,
			color_attachements_count, color_attachments,
			resolve_attachments_count, resolve_attachments,
			depth_stencil, N, att);
	}

	constexpr operator VkSubpassDescription() const
	{
		return check_consistency() ? VkSubpassDescription{ 0, pipeline_bind_point,
			input_attachements_count, input_attachments,
			color_attachements_count, color_attachments,
			resolve_attachments, depth_stencil,
			preserve_attachments_count, preserve_attachments } : throw ("Color and Resolve attachment mismatch");
	}

private:
	constexpr subpass_description(VkPipelineBindPoint binding_point,
		const uint32_t iac,
		const VkAttachmentReference* ia,
		const uint32_t cac,
		const VkAttachmentReference* ca,
		const uint32_t rac,
		const VkAttachmentReference* ra,
		const VkAttachmentReference* ds,
		const uint32_t pac,
		const uint32_t* pa) : pipeline_bind_point(binding_point), input_attachments(ia), input_attachements_count(iac),
		color_attachments(ca), resolve_attachments(ra), depth_stencil(ds), preserve_attachments(pa), color_attachements_count(cac),
		resolve_attachments_count(rac), preserve_attachments_count(pac)
	{ }

private:
	friend struct render_pass;
	constexpr bool check_consistency() const
	{
		return (color_attachements_count == resolve_attachments_count) || (resolve_attachments_count == 0);
	}
};
