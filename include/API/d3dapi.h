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
};

using command_list_storage_t = wrapper<ID3D12CommandAllocator>;
using command_list_t = Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>;
using device_t = Microsoft::WRL::ComPtr<ID3D12Device>;
using command_queue_t = wrapper<ID3D12CommandQueue>;
using buffer_t = Microsoft::WRL::ComPtr<ID3D12Resource>;
using image_t = Microsoft::WRL::ComPtr<ID3D12Resource>;
using descriptor_storage_t = Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>;
using pipeline_state_t = Microsoft::WRL::ComPtr<ID3D12PipelineState>;
using pipeline_layout_t = Microsoft::WRL::ComPtr<ID3D12RootSignature>;
using swap_chain_t = wrapper<IDXGISwapChain3>;
using render_pass_t = void*;
using clear_value_structure_t = D3D12_CLEAR_VALUE;

struct d3d12_framebuffer_t
{
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtt_heap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_heap;
    uint32_t NumRTT;
    bool hasDepthStencil;

	d3d12_framebuffer_t(device_t dev, const std::vector<std::tuple<image_t, irr::video::ECOLOR_FORMAT>> &render_targets);
	d3d12_framebuffer_t(device_t dev, const std::tuple<image_t, irr::video::ECOLOR_FORMAT> &depth_stencil_texture);
	d3d12_framebuffer_t(device_t dev, const std::vector<std::tuple<image_t, irr::video::ECOLOR_FORMAT>> &render_targets, const std::tuple<image_t, irr::video::ECOLOR_FORMAT> &depth_stencil_texture);
    ~d3d12_framebuffer_t();

    d3d12_framebuffer_t(d3d12_framebuffer_t&&) = delete;
    d3d12_framebuffer_t(const d3d12_framebuffer_t&) = delete;
};

using framebuffer_t = std::shared_ptr<d3d12_framebuffer_t>;

#include "GfxApi.h"
#include "../D3DAPI/pipeline_helpers.h"
#include "../D3DAPI/pipeline_layout_helpers.h"

void create_constant_buffer_view(device_t dev, descriptor_storage_t storage, uint32_t index, buffer_t buffer, uint32_t buffer_size);
void create_sampler(device_t dev, descriptor_storage_t storage, uint32_t index, SAMPLER_TYPE sampler_type);
void create_image_view(device_t dev, descriptor_storage_t storage, uint32_t index, image_t img, uint32_t mip_levels, irr::video::ECOLOR_FORMAT fmt, D3D12_SRV_DIMENSION dim);
void clear_color(device_t dev, command_list_t command_list, framebuffer_t framebuffer, const std::array<float, 4> &color);
void clear_depth_stencil(device_t dev, command_list_t command_list, framebuffer_t framebuffer, depth_stencil_aspect aspect, float depth, uint8_t stencil);
