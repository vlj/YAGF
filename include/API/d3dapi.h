// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl\client.h>
#include <vector>
#include <memory>
#include "..\Core\SColor.h"

template<typename T>
struct wrapper
{
	T* object;

	wrapper(T* ptr) : object(ptr)
	{}

	~wrapper()
	{
		object->Release();
	}

	operator T*()
	{
		return object;
	}

	T* 	operator ->()
	{
		return object;
	}
};

using command_list_storage_t = wrapper<ID3D12CommandAllocator>;
using command_list_t = wrapper<ID3D12GraphicsCommandList>;
using device_t = wrapper<ID3D12Device>;
using command_queue_t = wrapper<ID3D12CommandQueue>;
using buffer_t = wrapper<ID3D12Resource>;
using image_t = wrapper<ID3D12Resource>;

struct d3d12_descriptor_storage
{
	ID3D12DescriptorHeap *storage;
	uint32_t first_available_index;
};

using descriptor_storage_t = d3d12_descriptor_storage;
using pipeline_state_t = Microsoft::WRL::ComPtr<ID3D12PipelineState>;
using compute_pipeline_state_t = wrapper<ID3D12PipelineState>;
using pipeline_layout_t = Microsoft::WRL::ComPtr<ID3D12RootSignature>;
using swap_chain_t = wrapper<IDXGISwapChain3>;
using render_pass_t = void*;
using clear_value_structure_t = D3D12_CLEAR_VALUE;
using descriptor_set_layout = void;

enum class d3d12_texture_type
{
	texture2d,
	texturecube,
};

struct d3d12_image_view
{
	ID3D12Resource* image;
	DXGI_FORMAT format;
	d3d12_texture_type texture_type;
	uint16_t base_mipmap;
	uint16_t mipmap_count;
	uint16_t base_layer;
	uint16_t layer_count;
};

using image_view_t = d3d12_image_view;
using sampler_t = D3D12_SAMPLER_DESC;
using buffer_view_t = std::tuple<D3D12_SHADER_RESOURCE_VIEW_DESC, ID3D12Resource*>;

struct allocated_descriptor_set
{
private:
	D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;

public:
	allocated_descriptor_set() { }

	allocated_descriptor_set(D3D12_CPU_DESCRIPTOR_HANDLE c, D3D12_GPU_DESCRIPTOR_HANDLE g) :cpu_handle(c), gpu_handle(g)
	{ }

	operator D3D12_CPU_DESCRIPTOR_HANDLE() const
	{
		return cpu_handle;
	}

	operator D3D12_GPU_DESCRIPTOR_HANDLE() const
	{
		return gpu_handle;
	}
};

struct d3d12_framebuffer_t
{
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtt_heap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_heap;
    uint32_t NumRTT;
    bool hasDepthStencil;

	d3d12_framebuffer_t(device_t& dev, const std::vector<std::tuple<image_t&, irr::video::ECOLOR_FORMAT>> &render_targets);
	d3d12_framebuffer_t(device_t& dev, const std::tuple<image_t&, irr::video::ECOLOR_FORMAT> &depth_stencil_texture);
	d3d12_framebuffer_t(device_t& dev, const std::vector<std::tuple<image_t&, irr::video::ECOLOR_FORMAT>> &render_targets, const std::tuple<image_t&, irr::video::ECOLOR_FORMAT> &depth_stencil_texture);
    ~d3d12_framebuffer_t();

    d3d12_framebuffer_t(d3d12_framebuffer_t&&) = delete;
    d3d12_framebuffer_t(const d3d12_framebuffer_t&) = delete;
};

using framebuffer_t = std::shared_ptr<d3d12_framebuffer_t>;

#include "GfxApi.h"
#include "../D3DAPI/pipeline_helpers.h"
#include "../D3DAPI/pipeline_layout_helpers.h"

void create_image_view(device_t& dev, const allocated_descriptor_set& descriptor_set, uint32_t offset, image_t& img, uint32_t mip_levels, irr::video::ECOLOR_FORMAT fmt, D3D12_SRV_DIMENSION dim);
void clear_color(command_list_t& command_list, framebuffer_t framebuffer, const std::array<float, 4> &color);
void clear_depth_stencil(command_list_t& command_list, framebuffer_t framebuffer, float depth, uint8_t stencil);
