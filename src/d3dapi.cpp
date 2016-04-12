// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include "../include/API/d3dapi.h"
#include <d3dx12.h>
#include <algorithm>

#define CHECK_HRESULT(cmd) {HRESULT hr = cmd; if (hr != 0) throw;}


namespace
{
    D3D12_RESOURCE_STATES get_resource_state(RESOURCE_USAGE ru)
    {
        switch (ru)
        {
        case RESOURCE_USAGE::READ_GENERIC:
            return D3D12_RESOURCE_STATE_GENERIC_READ;
        case RESOURCE_USAGE::COPY_DEST:
            return D3D12_RESOURCE_STATE_COPY_DEST;
        case RESOURCE_USAGE::COPY_SRC:
            return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case RESOURCE_USAGE::PRESENT:
            return D3D12_RESOURCE_STATE_PRESENT;
        case RESOURCE_USAGE::RENDER_TARGET:
            return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case RESOURCE_USAGE::DEPTH_STENCIL:
            return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        }
        throw;
    }
}

command_list_storage_t create_command_storage(device_t dev)
{
    command_list_storage_t result;
    CHECK_HRESULT(dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(result.GetAddressOf())));
    return result;
}

command_list_t create_command_list(device_t dev, command_list_storage_t storage)
{
    command_list_t result;
    CHECK_HRESULT(dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, storage.Get(), nullptr, IID_PPV_ARGS(result.GetAddressOf())));
    return result;
}

buffer_t create_buffer(device_t dev, size_t size)
{
    buffer_t result;
    size_t real_size = std::max<size_t>(size, 256);
    CHECK_HRESULT(dev->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(real_size),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(result.GetAddressOf())));
    return result;
}

void* map_buffer(device_t dev, buffer_t buffer)
{
    void* result;
    CHECK_HRESULT(buffer->Map(0, nullptr, (void**)&result));
    return result;
}

void unmap_buffer(device_t dev, buffer_t buffer)
{
    buffer->Unmap(0, nullptr);
}

image_t create_image(device_t dev, DXGI_FORMAT format, uint32_t width, uint32_t height, uint16_t mipmap, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initial_state, D3D12_CLEAR_VALUE *clear_value)
{
    image_t result;
    CHECK_HRESULT(dev->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, mipmap, 1, 0, flags),
        initial_state,
        clear_value,
        IID_PPV_ARGS(result.GetAddressOf())));
    return result;
}

descriptor_storage_t create_descriptor_storage(device_t dev, uint32_t num_descriptors)
{
    descriptor_storage_t result;
    D3D12_DESCRIPTOR_HEAP_DESC heapdesc = {};
    heapdesc.NumDescriptors = num_descriptors;
    heapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    CHECK_HRESULT(dev->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(result.GetAddressOf())));
    return result;
}

void create_constant_buffer_view(device_t dev, descriptor_storage_t storage, uint32_t index, buffer_t buffer, uint32_t buffer_size)
{
    uint32_t stride = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
    desc.BufferLocation = buffer->GetGPUVirtualAddress();
    desc.SizeInBytes = std::max<uint32_t>(256, buffer_size);
    dev->CreateConstantBufferView(&desc, CD3DX12_CPU_DESCRIPTOR_HANDLE(storage->GetCPUDescriptorHandleForHeapStart()).Offset(index, stride));
}

descriptor_storage_t create_sampler_heap(device_t dev, uint32_t num_descriptors)
{
    descriptor_storage_t result;
    D3D12_DESCRIPTOR_HEAP_DESC heapdesc = {};
    heapdesc.NumDescriptors = num_descriptors;
    heapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    heapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    CHECK_HRESULT(dev->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(result.GetAddressOf())));
    return result;
}

void create_sampler(device_t dev, descriptor_storage_t storage, uint32_t index, SAMPLER_TYPE sampler_type)
{
    uint32_t stride = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    D3D12_SAMPLER_DESC samplerdesc = {};

    samplerdesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerdesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerdesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerdesc.MaxAnisotropy = 1;
    samplerdesc.MinLOD = 0;
    samplerdesc.MaxLOD = 1000;

    switch (sampler_type)
    {
    case SAMPLER_TYPE::TRILINEAR:
        samplerdesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        break;
    case SAMPLER_TYPE::BILINEAR:
        samplerdesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        samplerdesc.MaxLOD = 0;
        break;
    case SAMPLER_TYPE::NEAREST:
        samplerdesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        samplerdesc.MaxLOD = 0;
        break;
    case SAMPLER_TYPE::ANISOTROPIC:
        samplerdesc.Filter = D3D12_FILTER_ANISOTROPIC;
        samplerdesc.MaxAnisotropy = 16;
        break;
    }
    dev->CreateSampler(&samplerdesc, CD3DX12_CPU_DESCRIPTOR_HANDLE(storage->GetCPUDescriptorHandleForHeapStart()).Offset(index, stride));
}

void create_image_view(device_t dev, descriptor_storage_t storage, uint32_t index, image_t img)
{
    uint32_t stride = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.Texture2D.MipLevels = 9;
    dev->CreateShaderResourceView(img.Get(), &desc, CD3DX12_CPU_DESCRIPTOR_HANDLE(storage->GetCPUDescriptorHandleForHeapStart()).Offset(index, stride));
}

void start_command_list_recording(device_t dev, command_list_t command_list, command_list_storage_t storage)
{
    command_list->Reset(storage.Get(), nullptr);
}

void make_command_list_executable(device_t dev, command_list_t command_list)
{
    command_list->Close();
}


void set_pipeline_barrier(device_t dev, command_list_t command_list, image_t resource, RESOURCE_USAGE before, RESOURCE_USAGE after, uint32_t subresource)
{
    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), get_resource_state(before), get_resource_state(after), subresource));
}

void wait_for_command_queue_idle(device_t dev, command_queue_t command_queue)
{
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    CHECK_HRESULT(dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())));
    HANDLE completion_event = CreateEvent(0, FALSE, FALSE, 0);
    fence->SetEventOnCompletion(1, completion_event);
    command_queue->Signal(fence.Get(), 1);
    WaitForSingleObject(completion_event, INFINITE);
    CloseHandle(completion_event);
}

void present(device_t dev, swap_chain_t chain)
{
    CHECK_HRESULT(chain->Present(1, 0));
}

framebuffer_t::framebuffer_t()
{
}

framebuffer_t::framebuffer_t(device_t dev, const std::vector<ID3D12Resource*> &RTTs, ID3D12Resource *DepthStencil)
    : NumRTT(static_cast<uint32_t>(RTTs.size())), hasDepthStencil(DepthStencil != nullptr)
{
    D3D12_DESCRIPTOR_HEAP_DESC rtt_desc = {};
    rtt_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtt_desc.NumDescriptors = NumRTT;
    CHECK_HRESULT(dev->CreateDescriptorHeap(&rtt_desc, IID_PPV_ARGS(rtt_heap.GetAddressOf())));

    for (unsigned i = 0; i < RTTs.size(); i++)
    {
        D3D12_RENDER_TARGET_VIEW_DESC rttvd = {};
        rttvd.Format = RTTs[i]->GetDesc().Format;
        rttvd.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rttvd.Texture2D.MipSlice = 0;
        rttvd.Texture2D.PlaneSlice = 0;
        dev->CreateRenderTargetView(RTTs[i], &rttvd, CD3DX12_CPU_DESCRIPTOR_HANDLE(rtt_heap->GetCPUDescriptorHandleForHeapStart()).Offset(i, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)));
    }

    if (DepthStencil == nullptr)
        return;

    D3D12_DESCRIPTOR_HEAP_DESC dsv_desc = {};
    dsv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsv_desc.NumDescriptors = 1;
    CHECK_HRESULT(dev->CreateDescriptorHeap(&dsv_desc, IID_PPV_ARGS(dsv_heap.GetAddressOf())));
    D3D12_DEPTH_STENCIL_VIEW_DESC DSVDescription = {};
    DSVDescription.Format = DepthStencil->GetDesc().Format;
    DSVDescription.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    DSVDescription.Texture2D.MipSlice = 0;
    dev->CreateDepthStencilView(DepthStencil, &DSVDescription, dsv_heap->GetCPUDescriptorHandleForHeapStart());
}

framebuffer_t::~framebuffer_t()
{

}

framebuffer_t& framebuffer_t::operator=(framebuffer_t &&old)
{
    rtt_heap = old.rtt_heap;
    dsv_heap = old.dsv_heap;
    NumRTT = old.NumRTT;
    hasDepthStencil = old.hasDepthStencil;
    return *this;
}

void root_signature_builder::build_root_parameter(std::vector<D3D12_DESCRIPTOR_RANGE > &&ranges, D3D12_SHADER_VISIBILITY visibility)
{
    all_ranges.push_back(ranges);
    CD3DX12_ROOT_PARAMETER rp;
    rp.InitAsDescriptorTable(static_cast<uint32_t>(all_ranges.back().size()), all_ranges.back().data(), visibility);
    root_parameters.push_back(rp);
}

root_signature_builder::root_signature_builder(std::vector<std::tuple<std::vector<D3D12_DESCRIPTOR_RANGE >, D3D12_SHADER_VISIBILITY> > &&parameters, D3D12_ROOT_SIGNATURE_FLAGS flags)
{
    for (auto &&rp_parameter : parameters)
    {
        build_root_parameter(std::move(std::get<0>(rp_parameter)), std::get<1>(rp_parameter));
    }
    desc.Flags = flags;
    desc.NumParameters = static_cast<uint32_t>(root_parameters.size());
    desc.pParameters = root_parameters.data();
}

pipeline_layout_t root_signature_builder::get(device_t dev)
{
    pipeline_layout_t result;
    Microsoft::WRL::ComPtr<ID3DBlob> pSerializedRootSig;
    Microsoft::WRL::ComPtr<ID3DBlob> error;
    CHECK_HRESULT(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, pSerializedRootSig.GetAddressOf(), error.GetAddressOf()));

    CHECK_HRESULT(dev->CreateRootSignature(1,
        pSerializedRootSig->GetBufferPointer(), pSerializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(result.GetAddressOf())));
    return result;
}

device_t create_device()
{
    device_t dev;
    Microsoft::WRL::ComPtr<IDXGIFactory> fact;
    CHECK_HRESULT(CreateDXGIFactory(IID_PPV_ARGS(fact.GetAddressOf())));
    Microsoft::WRL::ComPtr<IDXGIAdapter> adaptater;
    CHECK_HRESULT(fact->EnumAdapters(0, adaptater.GetAddressOf()));
    CHECK_HRESULT(D3D12CreateDevice(adaptater.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(dev.GetAddressOf())));

    return dev;
}

command_queue_t create_graphic_command_queue(device_t dev)
{
    command_queue_t result;
    D3D12_COMMAND_QUEUE_DESC cmddesc = { D3D12_COMMAND_LIST_TYPE_DIRECT };
    CHECK_HRESULT(dev->CreateCommandQueue(&cmddesc, IID_PPV_ARGS(result.GetAddressOf())));
    return result;
}

swap_chain_t create_swap_chain(device_t dev, command_queue_t queue, HWND window)
{
    swap_chain_t chain;
    Microsoft::WRL::ComPtr<IDXGIFactory> fact;
    CHECK_HRESULT(CreateDXGIFactory(IID_PPV_ARGS(fact.GetAddressOf())));

    DXGI_SWAP_CHAIN_DESC swapChain = {};
    swapChain.BufferCount = 2;
    swapChain.Windowed = true;
    swapChain.OutputWindow = window;
    swapChain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChain.SampleDesc.Count = 1;
    swapChain.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    CHECK_HRESULT(fact->CreateSwapChain(queue.Get(), &swapChain, (IDXGISwapChain**)chain.GetAddressOf()));
    return chain;
}

std::vector<image_t> get_image_view_from_swap_chain(device_t dev, swap_chain_t chain)
{
    std::vector<image_t> result;
    for (int i = 0; i < 2; i++)
    {
        image_t back_buffer;
        CHECK_HRESULT(chain->GetBuffer(i, IID_PPV_ARGS(back_buffer.GetAddressOf())));
        back_buffer->SetName(L"BackBuffer");
        result.push_back(back_buffer);
    }
    return result;
}
