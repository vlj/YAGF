// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl\client.h>
#include <vector>

using command_list_storage_t = Microsoft::WRL::ComPtr<ID3D12CommandAllocator>;
using command_list_t = Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>;
using device_t = Microsoft::WRL::ComPtr<ID3D12Device>;
using command_queue_t = Microsoft::WRL::ComPtr<ID3D12CommandQueue>;
using buffer_t = Microsoft::WRL::ComPtr<ID3D12Resource>;
using image_t = Microsoft::WRL::ComPtr<ID3D12Resource>;
using descriptor_storage_t = Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>;
using pipeline_state_t = Microsoft::WRL::ComPtr<ID3D12PipelineState>;
using pipeline_layout_t = Microsoft::WRL::ComPtr<ID3D12RootSignature>;
using swap_chain_t = Microsoft::WRL::ComPtr<IDXGISwapChain3>;

struct framebuffer_t
{
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtt_heap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_heap;
    uint32_t NumRTT;
    bool hasDepthStencil;

    framebuffer_t();
    framebuffer_t(device_t dev, const std::vector<ID3D12Resource*> &RTTs, ID3D12Resource *DepthStencil);
    ~framebuffer_t();

    framebuffer_t(framebuffer_t&&) = delete;
    framebuffer_t& operator=(framebuffer_t&&);
};


struct root_signature_builder
{
private:
    std::vector<std::vector<D3D12_DESCRIPTOR_RANGE > > all_ranges;
    std::vector<D3D12_ROOT_PARAMETER> root_parameters;
    D3D12_ROOT_SIGNATURE_DESC desc = {};

    void build_root_parameter(std::vector<D3D12_DESCRIPTOR_RANGE > &&ranges, D3D12_SHADER_VISIBILITY visibility);
public:
    root_signature_builder(std::vector<std::tuple<std::vector<D3D12_DESCRIPTOR_RANGE >, D3D12_SHADER_VISIBILITY> > &&parameters, D3D12_ROOT_SIGNATURE_FLAGS flags);
    pipeline_layout_t get(device_t dev);
};

device_t create_device();
command_queue_t create_graphic_command_queue(device_t dev);
swap_chain_t create_swap_chain(device_t dev, command_queue_t queue, HWND window);
std::vector<image_t> get_image_view_from_swap_chain(device_t dev, swap_chain_t chain);

#include "GfxApi.h"