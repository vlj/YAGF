// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#include <vector>
#include <tuple>
#include <array>
#include <memory>
#include <gsl/gsl>
#include <variant>
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

		enum class E_POLYGON_MODE
		{
			EPM_FILL,
			EPM_LINE,
			EPM_POINT
		};

		enum class E_CULL_MODE
		{
			ECM_NONE,
			ECM_FRONT,
			ECM_BACK,
			ECM_FRONT_AND_BACK
		};

		enum class E_FRONT_FACE
		{
			EFF_CCW,
			EFF_CW
		};

		enum class E_SAMPLE_COUNT
		{
			ESC_1,
			ESC_2,
			ESC_4,
			ESC_8
		};

		enum class E_PRIMITIVE_TYPE
		{
			EPT_POINTS,
			EPT_LINE_STRIP,
			EPT_LINES,
			EPT_TRIANGLE_STRIP,
			EPT_TRIANGLES,
		};

		enum class E_COMPARE_FUNCTION
		{
			ECF_LESS,
			ECF_LEQUAL,
			ECF_NEVER,
		};

		enum class E_STENCIL_OP
		{
			ESO_KEEP,
		};

		enum class E_ASPECT
		{
			EA_COLOR,
			EA_DEPTH,
			EA_STENCIL,
			EA_DEPTH_STENCIL
		};

		enum class E_MEMORY_POOL
		{
			EMP_CPU_WRITEABLE,
			EMP_GPU_LOCAL,
			EMP_CPU_READABLE,
		};

		enum class E_TEXTURE_TYPE
		{
			ETT_2D,
			ETT_CUBE,
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
	undefined,
	uav,
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

enum buffer_flags
{
	none = 0,
	usage_uav = 0x1,
	usage_texel_buffer = 0x2,
    usage_buffer_transfer_src = 0x4,
};

enum class SAMPLER_TYPE
{
	NEAREST,
	BILINEAR_CLAMPED,
	TRILINEAR,
	ANISOTROPIC,
};

struct MipLevelData
{
	uint64_t Offset;
	uint32_t Width;
	uint32_t Height;
	uint32_t RowPitch;
};


enum class RESOURCE_VIEW
{
	CONSTANTS_BUFFER,
	INPUT_ATTACHMENT,
	SHADER_RESOURCE,
	TEXEL_BUFFER,
	SAMPLER,
	UAV_BUFFER,
	UAV_IMAGE,
};

enum class shader_stage
{
	vertex_shader,
	fragment_shader,
	all,
};



struct range_of_descriptors
{
	const RESOURCE_VIEW range_type;
	const uint32_t bind_point;
	const uint32_t count;

	constexpr range_of_descriptors(const RESOURCE_VIEW rt, const uint32_t bindpoint, const uint32_t range_size)
		: range_type(rt), bind_point(bindpoint), count(range_size)
	{}
};

struct descriptor_set_
{
	const range_of_descriptors *descriptors_ranges;
	const uint32_t count;
	shader_stage stage;

	constexpr descriptor_set_(const range_of_descriptors * ptr, const uint32_t cnt, shader_stage st) : descriptors_ranges(ptr), count(cnt), stage(st)
	{ }
};

template<size_t N>
constexpr descriptor_set_ descriptor_set(const range_of_descriptors (&arr)[N], const shader_stage stage)
{
	return descriptor_set_(arr, N, stage);
}

struct pipeline_vertex_attributes
{
	uint32_t location;
	irr::video::ECOLOR_FORMAT format;
	uint32_t binding;
	uint32_t stride;
	uint32_t offset;
};

struct compute_pipeline_state_description
{
	std::string compute_path;

	compute_pipeline_state_description set_compute_shader(const std::string& path)
	{
		compute_path = path;
		return *this;
	}
};

struct graphic_pipeline_state_description
{
	std::string vertex_path;
	std::string fragment_path;


	std::vector<pipeline_vertex_attributes> attributes;

	bool rasterization_depth_clamp_enable;
	bool rasterization_discard_enable;
	irr::video::E_POLYGON_MODE rasterization_polygon_mode;
	irr::video::E_CULL_MODE rasterization_cull_mode;
	irr::video::E_FRONT_FACE rasterization_front_face;
	bool rasterization_depth_bias_enable;
	float rasterization_depth_bias_constant_factor;
	float rasterization_depth_bias_clamp;
	float rasterization_depth_bias_slope_factor;
	float rasterization_line_width;
	bool rasterization_conservative_enable;

	bool multisample_multisample_enable;
	irr::video::E_SAMPLE_COUNT multisample_sample_count;
	float multisample_min_sample_shading;
	// sample mask ?
	bool multisample_alpha_to_coverage;
	bool multisample_alpha_to_one;

	irr::video::E_PRIMITIVE_TYPE input_assembly_topology;
	bool input_assembly_primitive_restart;

	bool depth_stencil_depth_test;
	bool depth_stencil_depth_write;
	irr::video::E_COMPARE_FUNCTION depth_stencil_depth_compare_op;
	bool depth_stencil_depth_clip_enable;
	bool depth_stencil_stencil_test;
	irr::video::E_STENCIL_OP depth_stencil_front_stencil_fail_op;
	irr::video::E_STENCIL_OP depth_stencil_front_stencil_depth_fail_op;
	irr::video::E_STENCIL_OP depth_stencil_front_stencil_pass_op;
	irr::video::E_COMPARE_FUNCTION depth_stencil_front_stencil_compare_op;
	irr::video::E_STENCIL_OP depth_stencil_back_stencil_fail_op;
	irr::video::E_STENCIL_OP depth_stencil_back_stencil_depth_fail_op;
	irr::video::E_STENCIL_OP depth_stencil_back_stencil_pass_op;
	irr::video::E_COMPARE_FUNCTION depth_stencil_back_stencil_compare_op;
	float depth_stencil_min_depth_clip;
	float depth_stencil_max_depth_clip;

	static graphic_pipeline_state_description get()
	{
		return graphic_pipeline_state_description(false, false, irr::video::E_POLYGON_MODE::EPM_FILL, irr::video::E_CULL_MODE::ECM_BACK, irr::video::E_FRONT_FACE::EFF_CW, false, 0.f, 0.f, 0.f, 1.f, false,
			false, irr::video::E_SAMPLE_COUNT::ESC_1, 0.f, false, false, irr::video::E_PRIMITIVE_TYPE::EPT_TRIANGLES, false, true, true, irr::video::E_COMPARE_FUNCTION::ECF_LESS, false, false,
			irr::video::E_STENCIL_OP::ESO_KEEP, irr::video::E_STENCIL_OP::ESO_KEEP, irr::video::E_STENCIL_OP::ESO_KEEP, irr::video::E_COMPARE_FUNCTION::ECF_NEVER,
			irr::video::E_STENCIL_OP::ESO_KEEP, irr::video::E_STENCIL_OP::ESO_KEEP, irr::video::E_STENCIL_OP::ESO_KEEP, irr::video::E_COMPARE_FUNCTION::ECF_NEVER,
			0.f, 1.f);
	}

	graphic_pipeline_state_description set_depth_compare_function(irr::video::E_COMPARE_FUNCTION depth_compare) const
	{
		return graphic_pipeline_state_description(rasterization_depth_clamp_enable,
			rasterization_discard_enable,
			rasterization_polygon_mode,
			rasterization_cull_mode,
			rasterization_front_face,
			rasterization_depth_bias_enable,
			rasterization_depth_bias_constant_factor,
			rasterization_depth_bias_clamp,
			rasterization_depth_bias_slope_factor,
			rasterization_line_width,
			rasterization_conservative_enable,
			multisample_multisample_enable,
			multisample_sample_count,
			multisample_min_sample_shading,
			multisample_alpha_to_coverage,
			multisample_alpha_to_one,
			input_assembly_topology,
			input_assembly_primitive_restart,
			depth_stencil_depth_test,
			depth_stencil_depth_write,
			depth_compare,
			depth_stencil_depth_clip_enable,
			depth_stencil_stencil_test,
			depth_stencil_front_stencil_fail_op,
			depth_stencil_front_stencil_depth_fail_op,
			depth_stencil_front_stencil_pass_op,
			depth_stencil_front_stencil_compare_op,
			depth_stencil_back_stencil_fail_op,
			depth_stencil_back_stencil_depth_fail_op,
			depth_stencil_back_stencil_pass_op,
			depth_stencil_back_stencil_compare_op,
			depth_stencil_min_depth_clip,
			depth_stencil_max_depth_clip);
	}

	graphic_pipeline_state_description set_depth_write(bool depthwrite) const
	{
		return graphic_pipeline_state_description(rasterization_depth_clamp_enable,
			rasterization_discard_enable,
			rasterization_polygon_mode,
			rasterization_cull_mode,
			rasterization_front_face,
			rasterization_depth_bias_enable,
			rasterization_depth_bias_constant_factor,
			rasterization_depth_bias_clamp,
			rasterization_depth_bias_slope_factor,
			rasterization_line_width,
			rasterization_conservative_enable,
			multisample_multisample_enable,
			multisample_sample_count,
			multisample_min_sample_shading,
			multisample_alpha_to_coverage,
			multisample_alpha_to_one,
			input_assembly_topology,
			input_assembly_primitive_restart,
			depth_stencil_depth_test,
			depthwrite,
			depth_stencil_depth_compare_op,
			depth_stencil_depth_clip_enable,
			depth_stencil_stencil_test,
			depth_stencil_front_stencil_fail_op,
			depth_stencil_front_stencil_depth_fail_op,
			depth_stencil_front_stencil_pass_op,
			depth_stencil_front_stencil_compare_op,
			depth_stencil_back_stencil_fail_op,
			depth_stencil_back_stencil_depth_fail_op,
			depth_stencil_back_stencil_pass_op,
			depth_stencil_back_stencil_compare_op,
			depth_stencil_min_depth_clip,
			depth_stencil_max_depth_clip);
	}

	graphic_pipeline_state_description set_depth_test(bool depth_test) const
	{
		return graphic_pipeline_state_description(rasterization_depth_clamp_enable,
			rasterization_discard_enable,
			rasterization_polygon_mode,
			rasterization_cull_mode,
			rasterization_front_face,
			rasterization_depth_bias_enable,
			rasterization_depth_bias_constant_factor,
			rasterization_depth_bias_clamp,
			rasterization_depth_bias_slope_factor,
			rasterization_line_width,
			rasterization_conservative_enable,
			multisample_multisample_enable,
			multisample_sample_count,
			multisample_min_sample_shading,
			multisample_alpha_to_coverage,
			multisample_alpha_to_one,
			input_assembly_topology,
			input_assembly_primitive_restart,
			depth_test,
			depth_stencil_depth_write,
			depth_stencil_depth_compare_op,
			depth_stencil_depth_clip_enable,
			depth_stencil_stencil_test,
			depth_stencil_front_stencil_fail_op,
			depth_stencil_front_stencil_depth_fail_op,
			depth_stencil_front_stencil_pass_op,
			depth_stencil_front_stencil_compare_op,
			depth_stencil_back_stencil_fail_op,
			depth_stencil_back_stencil_depth_fail_op,
			depth_stencil_back_stencil_pass_op,
			depth_stencil_back_stencil_compare_op,
			depth_stencil_min_depth_clip,
			depth_stencil_max_depth_clip);
	}

	graphic_pipeline_state_description set_vertex_shader(const std::string& path)
	{
		vertex_path = path;
		return *this;
	}

	graphic_pipeline_state_description set_fragment_shader(const std::string& path)
	{
		fragment_path = path;
		return *this;
	}

	graphic_pipeline_state_description set_vertex_attributes(const gsl::span<pipeline_vertex_attributes> &attributes_)
	{
		attributes = std::vector<pipeline_vertex_attributes>(attributes_.begin(), attributes_.end());
		return *this;
	}

	graphic_pipeline_state_description()
	{

	}

private:
	graphic_pipeline_state_description(
		bool depth_clamp_enable,
		bool rasterizer_discard_enable,
		irr::video::E_POLYGON_MODE polygon_mode,
		irr::video::E_CULL_MODE cull_mode,
		irr::video::E_FRONT_FACE front_face,
		bool depth_bias_enable,
		float depth_bias_constant_factor,
		float depth_bias_clamp,
		float depth_bias_slope_factor,
		float line_width,
		bool conservative_enable,
		bool multisample_enable,
		irr::video::E_SAMPLE_COUNT sample_count,
		float min_sample_shading,
		bool alpha_to_coverage,
		bool alpha_to_one,
		irr::video::E_PRIMITIVE_TYPE topology,
		bool primitive_restart,
		bool depth_test,
		bool depth_write,
		irr::video::E_COMPARE_FUNCTION depth_compare_op,
		bool depth_clip_enable,
		bool stencil_test,
		irr::video::E_STENCIL_OP front_stencil_fail_op,
		irr::video::E_STENCIL_OP front_stencil_depth_fail_op,
		irr::video::E_STENCIL_OP front_stencil_pass_op,
		irr::video::E_COMPARE_FUNCTION front_stencil_compare_op,
		irr::video::E_STENCIL_OP back_stencil_fail_op,
		irr::video::E_STENCIL_OP back_stencil_depth_fail_op,
		irr::video::E_STENCIL_OP back_stencil_pass_op,
		irr::video::E_COMPARE_FUNCTION back_stencil_compare_op,
		float min_depth_clip,
		float max_depth_clip
	)
		: rasterization_depth_clamp_enable(depth_clamp_enable),
		rasterization_discard_enable(rasterizer_discard_enable),
		rasterization_polygon_mode(polygon_mode),
		rasterization_cull_mode(cull_mode),
		rasterization_front_face(front_face),
		rasterization_depth_bias_enable(depth_bias_enable),
		rasterization_depth_bias_constant_factor(depth_bias_constant_factor),
		rasterization_depth_bias_clamp(depth_bias_clamp),
		rasterization_depth_bias_slope_factor(depth_bias_slope_factor),
		rasterization_line_width(line_width),
		rasterization_conservative_enable(conservative_enable),
		multisample_multisample_enable(multisample_enable),
		multisample_sample_count(sample_count),
		multisample_min_sample_shading(min_sample_shading),
		multisample_alpha_to_coverage(alpha_to_coverage),
		multisample_alpha_to_one(alpha_to_one),
		input_assembly_topology(topology),
		input_assembly_primitive_restart(primitive_restart),
		depth_stencil_depth_test(depth_test),
		depth_stencil_depth_write(depth_write),
		depth_stencil_depth_compare_op(depth_compare_op),
		depth_stencil_depth_clip_enable(depth_clip_enable),
		depth_stencil_stencil_test(stencil_test),
		depth_stencil_front_stencil_fail_op(front_stencil_fail_op),
		depth_stencil_front_stencil_depth_fail_op(front_stencil_depth_fail_op),
		depth_stencil_front_stencil_pass_op(front_stencil_pass_op),
		depth_stencil_front_stencil_compare_op(front_stencil_compare_op),
		depth_stencil_back_stencil_fail_op(back_stencil_fail_op),
		depth_stencil_back_stencil_depth_fail_op(back_stencil_depth_fail_op),
		depth_stencil_back_stencil_pass_op(back_stencil_pass_op),
		depth_stencil_back_stencil_compare_op(back_stencil_compare_op),
		depth_stencil_min_depth_clip(min_depth_clip),
		depth_stencil_max_depth_clip(max_depth_clip)
	{ }
};

struct framebuffer_t {
	virtual ~framebuffer_t() {}
};

struct pipeline_state_t {
	virtual ~pipeline_state_t() {}
};

struct compute_pipeline_state_t {
	virtual ~compute_pipeline_state_t() {}
};

struct pipeline_layout_t {
	virtual ~pipeline_layout_t() {}
};

struct render_pass_t {
	virtual ~render_pass_t() {}
};

using clear_value_t = std::variant<std::array<float, 4>, std::tuple<float, uint8_t> >;

struct image_view_t {
	virtual ~image_view_t() {}
};

struct sampler_t {
	virtual ~sampler_t() {}
};

struct buffer_view_t {
	virtual ~buffer_view_t() {}
};

struct allocated_descriptor_set {
};

struct descriptor_set_layout {
	virtual ~descriptor_set_layout() {}
};

struct buffer_t {
	virtual void* map_buffer() = 0;
	virtual void unmap_buffer() = 0;
};

struct image_t {
	virtual ~image_t() {}
};

struct descriptor_storage_t {
	virtual std::unique_ptr<allocated_descriptor_set> allocate_descriptor_set_from_cbv_srv_uav_heap(uint32_t starting_index, const std::vector<descriptor_set_layout*> layouts, uint32_t descriptors_count) = 0;
	virtual std::unique_ptr<allocated_descriptor_set> allocate_descriptor_set_from_sampler_heap(uint32_t starting_index, const std::vector<descriptor_set_layout*> layouts, uint32_t descriptors_count) = 0;
};

struct command_list_t {
	virtual void bind_graphic_descriptor(uint32_t bindpoint, const allocated_descriptor_set& descriptor_set, pipeline_layout_t& sig) = 0;
	virtual void bind_compute_descriptor(uint32_t bindpoint, const allocated_descriptor_set& descriptor_set, pipeline_layout_t& sig) = 0;
	virtual void copy_buffer_to_image_subresource(image_t& destination_image, uint32_t destination_subresource, buffer_t& source, uint64_t offset_in_buffer,
		uint32_t width, uint32_t height, uint32_t row_pitch, irr::video::ECOLOR_FORMAT format) = 0;
	virtual void set_pipeline_barrier(image_t& resource, RESOURCE_USAGE before, RESOURCE_USAGE after, uint32_t subresource, irr::video::E_ASPECT) = 0;
	virtual void set_uav_flush(image_t& resource) = 0;

	virtual void set_viewport(float x, float width, float y, float height, float min_depth, float max_depth) = 0;
	virtual void set_scissor(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom) = 0;
	virtual void set_graphic_pipeline(pipeline_state_t& pipeline) = 0;
	virtual void set_graphic_pipeline_layout(pipeline_layout_t& sig) = 0;
	virtual void set_compute_pipeline(compute_pipeline_state_t& pipeline) = 0;
	virtual void set_compute_pipeline_layout(pipeline_layout_t& sig) = 0;
	virtual void set_descriptor_storage_referenced(descriptor_storage_t& main_heap, descriptor_storage_t* sampler_heap = nullptr) = 0;
	virtual void bind_index_buffer(buffer_t& buffer, uint64_t offset, uint32_t size, irr::video::E_INDEX_TYPE type) = 0;
	virtual void bind_vertex_buffers(uint32_t first_bind, const std::vector<std::tuple<buffer_t&, uint64_t, uint32_t, uint32_t> > &buffer_offset_stride_size) = 0;
	virtual void draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t base_index, int32_t base_vertex, uint32_t base_instance) = 0;
	virtual void draw_non_indexed(uint32_t vertex_count, uint32_t instance_count, int32_t base_vertex, uint32_t base_instance) = 0;
	virtual void dispatch(uint32_t x, uint32_t y, uint32_t z) = 0;
	virtual void copy_buffer(buffer_t& src, uint64_t src_offset, buffer_t& dst, uint64_t dst_offset, uint64_t size) = 0;

	virtual void begin_renderpass(render_pass_t& rp, framebuffer_t& fbo,
		gsl::span<clear_value_t> clear_values,
		uint32_t width, uint32_t height) = 0;
	virtual void next_subpass() = 0;
	virtual void end_renderpass() = 0;

	virtual void make_command_list_executable() = 0;
	virtual void start_command_list_recording(struct command_list_storage_t& storage) = 0;
};


struct command_list_storage_t {
	virtual std::unique_ptr<command_list_t> create_command_list() = 0;
	virtual void reset_command_list_storage() = 0;
	virtual ~command_list_storage_t() {}
};

struct command_queue_t {
	virtual void submit_executable_command_list(command_list_t& command_list) = 0;
	virtual void wait_for_command_queue_idle() = 0;
};

struct swap_chain_t {
	virtual ~swap_chain_t() {}
	virtual uint32_t get_next_backbuffer_id() = 0;
	virtual std::vector<std::unique_ptr<image_t>> get_image_view_from_swap_chain() = 0;
	virtual void present(command_queue_t& cmdqueue, uint32_t backbuffer_index) = 0;
};

struct device_t {
	virtual std::unique_ptr<command_list_storage_t> create_command_storage() = 0;
	virtual std::unique_ptr<buffer_t> create_buffer(size_t size, irr::video::E_MEMORY_POOL memory_pool, uint32_t flags) = 0;
	virtual std::unique_ptr<buffer_view_t> create_buffer_view(buffer_t&, irr::video::ECOLOR_FORMAT, uint64_t offset, uint32_t size) = 0;
	virtual void set_constant_buffer_view(const allocated_descriptor_set& descriptor_set, uint32_t offset_in_set, uint32_t binding_location, buffer_t& buffer, uint32_t buffer_size, uint64_t offset_in_buffer = 0) = 0;
	virtual void set_uniform_texel_buffer_view(const allocated_descriptor_set& descriptor_set, uint32_t offset_in_set, uint32_t binding_location, buffer_view_t& buffer_view) = 0;
	virtual void set_uav_buffer_view(const allocated_descriptor_set& descriptor_set, uint32_t offset_in_set, uint32_t binding_location, buffer_t& buffer, uint64_t offset, uint32_t size) = 0;
	virtual std::unique_ptr<image_t> create_image(irr::video::ECOLOR_FORMAT format, uint32_t width, uint32_t height, uint16_t mipmap, uint32_t layers, uint32_t flags, clear_value_t *clear_value) = 0;
	virtual std::unique_ptr<image_view_t> create_image_view(image_t& img, irr::video::ECOLOR_FORMAT fmt, uint16_t base_mipmap, uint16_t mipmap_count, uint16_t base_layer, uint16_t layer_count, irr::video::E_TEXTURE_TYPE texture_type, irr::video::E_ASPECT aspect = irr::video::E_ASPECT::EA_COLOR) = 0;
	virtual void set_image_view(const allocated_descriptor_set& descriptor_set, uint32_t offset, uint32_t binding_location, image_view_t& img_view) = 0;
	virtual void set_input_attachment(const allocated_descriptor_set& descriptor_set, uint32_t offset, uint32_t binding_location, image_view_t& img_view) = 0;
	virtual void set_uav_image_view(const allocated_descriptor_set& descriptor_set, uint32_t offset, uint32_t binding_location, image_view_t& img_view) = 0;
	virtual void set_sampler(const allocated_descriptor_set& descriptor_set, uint32_t offset, uint32_t binding_location, sampler_t& sampler) = 0;
	virtual std::unique_ptr<sampler_t> create_sampler(SAMPLER_TYPE sampler_type) = 0;
	virtual std::unique_ptr<descriptor_storage_t> create_descriptor_storage(uint32_t num_sets, const std::vector<std::tuple<RESOURCE_VIEW, uint32_t> > &num_descriptors) = 0;
	virtual std::unique_ptr<framebuffer_t> create_frame_buffer(gsl::span<const image_view_t*> render_targets, uint32_t width, uint32_t height, render_pass_t* render_pass) = 0;
	virtual std::unique_ptr<framebuffer_t> create_frame_buffer(gsl::span<const image_view_t*> render_targets, const image_view_t& depth_stencil_texture, uint32_t width, uint32_t height, render_pass_t* render_pass) = 0;
	virtual std::unique_ptr<descriptor_set_layout> get_object_descriptor_set(const descriptor_set_ &ds) = 0;
	virtual std::unique_ptr<pipeline_state_t> create_graphic_pso(const graphic_pipeline_state_description&) = 0;
	virtual std::unique_ptr<compute_pipeline_state_t> create_compute_pso(const compute_pipeline_state_description&) = 0;
	virtual std::unique_ptr<pipeline_layout_t> create_pipeline_layout(gsl::span<const descriptor_set_layout *>) = 0;

	virtual ~device_t() {};
};

clear_value_t get_clear_value(irr::video::ECOLOR_FORMAT format, float depth, uint8_t stencil);
clear_value_t get_clear_value(irr::video::ECOLOR_FORMAT format, const std::array<float,4> &color);
