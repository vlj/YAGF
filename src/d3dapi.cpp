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
		case RESOURCE_USAGE::DEPTH_WRITE:
			return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		case RESOURCE_USAGE::undefined:
			return D3D12_RESOURCE_STATE_GENERIC_READ;
		case RESOURCE_USAGE::uav:
			return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}
		throw;
	}

	D3D12_CLEAR_FLAGS get_clear_flag_from_aspect(depth_stencil_aspect aspect)
	{
		switch (aspect)
		{
		case depth_stencil_aspect::depth_and_stencil: return D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL;
		case depth_stencil_aspect::depth_only: return D3D12_CLEAR_FLAG_DEPTH;
		case depth_stencil_aspect::stencil_only: return D3D12_CLEAR_FLAG_STENCIL;
		}
		throw;
	}

	DXGI_FORMAT get_index_type(irr::video::E_INDEX_TYPE type)
	{
		switch (type)
		{
		case irr::video::E_INDEX_TYPE::EIT_16BIT: return DXGI_FORMAT_R16_UINT;
		case irr::video::E_INDEX_TYPE::EIT_32BIT: return DXGI_FORMAT_R32_UINT;
		}
		throw;
	}

	DXGI_FORMAT get_dxgi_format(irr::video::ECOLOR_FORMAT fmt)
	{
		switch (fmt)
		{
		case irr::video::ECF_R8G8B8A8_UNORM:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case irr::video::ECF_R8G8B8A8_UNORM_SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case irr::video::ECF_R16G16B16A16F:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case irr::video::ECF_R32G32B32A32F:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case irr::video::ECF_A8R8G8B8:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case irr::video::ECF_BC1_UNORM:
			return DXGI_FORMAT_BC1_UNORM;
		case irr::video::ECF_BC1_UNORM_SRGB:
			return DXGI_FORMAT_BC1_UNORM_SRGB;
		case irr::video::ECF_BC2_UNORM:
			return DXGI_FORMAT_BC2_UNORM;
		case irr::video::ECF_BC2_UNORM_SRGB:
			return DXGI_FORMAT_BC2_UNORM_SRGB;
		case irr::video::ECF_BC3_UNORM:
			return DXGI_FORMAT_BC3_UNORM;
		case irr::video::ECF_BC3_UNORM_SRGB:
			return DXGI_FORMAT_BC3_UNORM_SRGB;
		case irr::video::ECF_BC4_UNORM:
			return DXGI_FORMAT_BC4_UNORM;
		case irr::video::ECF_BC4_SNORM:
			return DXGI_FORMAT_BC4_SNORM;
		case irr::video::ECF_BC5_UNORM:
			return DXGI_FORMAT_BC5_UNORM;
		case irr::video::ECF_BC5_SNORM:
			return DXGI_FORMAT_BC5_SNORM;
		case irr::video::D24U8:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		}
		return DXGI_FORMAT_UNKNOWN;
	}


	DXGI_FORMAT get_dxgi_samplable_format(irr::video::ECOLOR_FORMAT fmt)
	{
		switch (fmt)
		{
		case irr::video::ECF_R8G8B8A8_UNORM:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case irr::video::ECF_R8G8B8A8_UNORM_SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case irr::video::ECF_R16G16B16A16F:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case irr::video::ECF_R32G32B32A32F:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case irr::video::ECF_A8R8G8B8:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case irr::video::ECF_BC1_UNORM:
			return DXGI_FORMAT_BC1_UNORM;
		case irr::video::ECF_BC1_UNORM_SRGB:
			return DXGI_FORMAT_BC1_UNORM_SRGB;
		case irr::video::ECF_BC2_UNORM:
			return DXGI_FORMAT_BC2_UNORM;
		case irr::video::ECF_BC2_UNORM_SRGB:
			return DXGI_FORMAT_BC2_UNORM_SRGB;
		case irr::video::ECF_BC3_UNORM:
			return DXGI_FORMAT_BC3_UNORM;
		case irr::video::ECF_BC3_UNORM_SRGB:
			return DXGI_FORMAT_BC3_UNORM_SRGB;
		case irr::video::ECF_BC4_UNORM:
			return DXGI_FORMAT_BC4_UNORM;
		case irr::video::ECF_BC4_SNORM:
			return DXGI_FORMAT_BC4_SNORM;
		case irr::video::ECF_BC5_UNORM:
			return DXGI_FORMAT_BC5_UNORM;
		case irr::video::ECF_BC5_SNORM:
			return DXGI_FORMAT_BC5_SNORM;
		case irr::video::D24U8:
			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		}
		return DXGI_FORMAT_UNKNOWN;
	}
}

std::unique_ptr<command_list_storage_t> create_command_storage(device_t& dev)
{
	ID3D12CommandAllocator *result;
	CHECK_HRESULT(dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&result)));
	return std::make_unique<command_list_storage_t>(result);
}

std::unique_ptr<command_list_t> create_command_list(device_t& dev, command_list_storage_t& storage)
{
	ID3D12GraphicsCommandList* result;
	CHECK_HRESULT(dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, storage, nullptr, IID_PPV_ARGS(&result)));
	CHECK_HRESULT(result->Close());
	return std::make_unique<command_list_t>(result);
}

namespace
{
	D3D12_HEAP_TYPE get_heap_type(irr::video::E_MEMORY_POOL memory_pool)
	{
		switch (memory_pool)
		{
		case irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE: return D3D12_HEAP_TYPE_UPLOAD;
		case irr::video::E_MEMORY_POOL::EMP_GPU_LOCAL: return D3D12_HEAP_TYPE_DEFAULT;
		case irr::video::E_MEMORY_POOL::EMP_CPU_READABLE: return D3D12_HEAP_TYPE_READBACK;
		}
		throw;
	}

	D3D12_RESOURCE_FLAGS get_buffer_flags(uint32_t flags)
	{
		D3D12_RESOURCE_FLAGS result = D3D12_RESOURCE_FLAG_NONE;
		if (flags & usage_uav)
			result |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		return result;
	}
}

std::unique_ptr<buffer_t> create_buffer(device_t& dev, size_t size, irr::video::E_MEMORY_POOL memory_pool , uint32_t flags)
{
	ID3D12Resource* result;
	D3D12_RESOURCE_STATES initial_state = (memory_pool == irr::video::E_MEMORY_POOL::EMP_CPU_READABLE) ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ;
	size_t real_size = std::max<size_t>(size, 256);
	CHECK_HRESULT(dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(get_heap_type(memory_pool)),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(real_size, get_buffer_flags(flags)),
		initial_state,
		nullptr,
		IID_PPV_ARGS(&result)));

	return std::make_unique<buffer_t>(result);
}

void* map_buffer(device_t& dev, buffer_t& buffer)
{
	void* result;
	CHECK_HRESULT(buffer->Map(0, nullptr, (void**)&result));
	return result;
}

void unmap_buffer(device_t& dev, buffer_t& buffer)
{
	buffer->Unmap(0, nullptr);
}

namespace
{
	D3D12_RESOURCE_FLAGS get_resource_flags(uint32_t flag)
	{
		D3D12_RESOURCE_FLAGS result = D3D12_RESOURCE_FLAG_NONE;
		if (flag & usage_render_target)
			result |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		if (flag & usage_depth_stencil)
			result |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		if (flag & usage_uav)
			result |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		return result;
	}
}

std::unique_ptr<image_t> create_image(device_t& dev, irr::video::ECOLOR_FORMAT format, uint32_t width, uint32_t height, uint16_t mipmap, uint32_t layers, uint32_t flags, clear_value_structure_t *clear_value)
{
	ID3D12Resource* result;
	CHECK_HRESULT(dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(get_dxgi_format(format), width, height, layers, mipmap, 1, 0, get_resource_flags(flags)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		clear_value,
		IID_PPV_ARGS(&result)));
	return std::make_unique<image_t>(result);
}

void copy_buffer_to_image_subresource(command_list_t& list, image_t& destination_image, uint32_t destination_subresource, buffer_t& source, uint64_t offset_in_buffer,
	uint32_t width, uint32_t height, uint32_t row_pitch, irr::video::ECOLOR_FORMAT format)
{
	list->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(destination_image, destination_subresource), 0, 0, 0,
		&CD3DX12_TEXTURE_COPY_LOCATION(source, { offset_in_buffer,{ get_dxgi_format(format), width, height, 1, row_pitch } }),
		&CD3DX12_BOX(0, 0, width, height));
}

framebuffer_t create_frame_buffer(device_t& dev, std::vector<std::tuple<image_t&, irr::video::ECOLOR_FORMAT>> render_targets, std::tuple<image_t&, irr::video::ECOLOR_FORMAT> depth_stencil_texture, uint32_t, uint32_t, render_pass_t*)
{
	return std::make_shared<d3d12_framebuffer_t>(dev, render_targets, depth_stencil_texture);
}

void create_constant_buffer_view(device_t& dev, const allocated_descriptor_set& descriptor_set, uint32_t offset_in_set, buffer_t& buffer, uint32_t buffer_size)
{
	uint32_t stride = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.BufferLocation = buffer->GetGPUVirtualAddress();
	desc.SizeInBytes = std::max<uint32_t>(256, buffer_size);
	dev->CreateConstantBufferView(&desc, CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptor_set).Offset(offset_in_set, stride));
}

void reset_command_list_storage(device_t&, command_list_storage_t& storage)
{
	storage->Reset();
}

namespace
{
	D3D12_DESCRIPTOR_HEAP_TYPE get_descriptor_heap_type(RESOURCE_VIEW type)
	{
		switch (type)
		{
		case RESOURCE_VIEW::CONSTANTS_BUFFER:
		case RESOURCE_VIEW::UAV_BUFFER:
		case RESOURCE_VIEW::UAV_IMAGE:
		case RESOURCE_VIEW::SHADER_RESOURCE: return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		case RESOURCE_VIEW::SAMPLER: return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		}
		throw;
	}
}

std::unique_ptr<descriptor_storage_t> create_descriptor_storage(device_t& dev, uint32_t num_sets, const std::vector<std::tuple<RESOURCE_VIEW, uint32_t> > &num_descriptors)
{
	uint32_t total_size = 0;
	for (const auto &set_size : num_descriptors)
	{
		total_size += std::get<1>(set_size);
	}
	ID3D12DescriptorHeap* result;
	D3D12_DESCRIPTOR_HEAP_DESC heapdesc = {};
	heapdesc.NumDescriptors = total_size;
	heapdesc.Type = get_descriptor_heap_type(std::get<0>(num_descriptors[0]));
	heapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	CHECK_HRESULT(dev->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(&result)));
	return std::make_unique<descriptor_storage_t>(result);
}

void create_sampler(device_t& dev, const allocated_descriptor_set& descriptor_set, uint32_t offset_in_set, SAMPLER_TYPE sampler_type)
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
	dev->CreateSampler(&samplerdesc, CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptor_set).Offset(offset_in_set, stride));
}

void create_image_view(device_t& dev, const allocated_descriptor_set& descriptor_set, uint32_t offset, image_t& img, uint32_t mip_levels, irr::video::ECOLOR_FORMAT fmt, D3D12_SRV_DIMENSION dim)
{
	uint32_t stride = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.ViewDimension = dim;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	if (dim == D3D12_SRV_DIMENSION_TEXTURECUBE)
		desc.TextureCube.MipLevels = mip_levels;
	if (dim == D3D12_SRV_DIMENSION_TEXTURE2D)
		desc.Texture2D.MipLevels = mip_levels;
	desc.Format = get_dxgi_samplable_format(fmt);
	dev->CreateShaderResourceView(img, &desc, CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptor_set).Offset(offset, stride));
}

void create_buffer_uav_view(device_t& dev, descriptor_storage_t& storage, uint32_t index, buffer_t& buffer, uint32_t size)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
	desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	desc.Format = DXGI_FORMAT_R32_TYPELESS;
	desc.Buffer.NumElements = size / 4;
	desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

	dev->CreateUnorderedAccessView(buffer, nullptr, &desc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(storage->GetCPUDescriptorHandleForHeapStart())
		.Offset(index, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));
}

void start_command_list_recording(command_list_t& command_list, command_list_storage_t& storage)
{
	command_list->Reset(storage, nullptr);
}

void make_command_list_executable(command_list_t& command_list)
{
	CHECK_HRESULT(command_list->Close());
}


void set_pipeline_barrier(command_list_t& command_list, image_t& resource, RESOURCE_USAGE before, RESOURCE_USAGE after, uint32_t subresource, irr::video::E_ASPECT)
{
	command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource, get_resource_state(before), get_resource_state(after), subresource));
}

void clear_color(command_list_t& command_list, framebuffer_t framebuffer, const std::array<float, 4> &color)
{
	command_list->ClearRenderTargetView(framebuffer->rtt_heap->GetCPUDescriptorHandleForHeapStart(), color.data(), 0, nullptr);
}

void clear_depth_stencil(command_list_t& command_list, framebuffer_t framebuffer, depth_stencil_aspect aspect, float depth, uint8_t stencil)
{
	command_list->ClearDepthStencilView(framebuffer->dsv_heap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, depth, stencil, 0, nullptr);

}

void set_viewport(command_list_t& command_list, float x, float width, float y, float height, float min_depth, float max_depth)
{
	D3D12_VIEWPORT view = {};
	view.Height = height;
	view.Width = width;
	view.TopLeftX = x;
	view.TopLeftY = y;
	view.MinDepth = min_depth;
	view.MaxDepth = max_depth;

	command_list->RSSetViewports(1, &view);
}

void set_scissor(command_list_t& command_list, uint32_t left, uint32_t right, uint32_t top, uint32_t bottom)
{
	D3D12_RECT rect = {};
	rect.left = left;
	rect.top = top;
	rect.bottom = bottom;
	rect.right = right;

	command_list->RSSetScissorRects(1, &rect);
}

void bind_index_buffer(command_list_t& command_list, buffer_t& buffer, uint64_t offset, uint32_t size, irr::video::E_INDEX_TYPE type)
{
	D3D12_INDEX_BUFFER_VIEW index_buffer_view = {};
	index_buffer_view.BufferLocation = buffer->GetGPUVirtualAddress() + offset;
	index_buffer_view.SizeInBytes = size;
	index_buffer_view.Format = get_index_type(type);
	command_list->IASetIndexBuffer(&index_buffer_view);
}

void bind_vertex_buffers(command_list_t& commandlist, uint32_t first_bind, const std::vector<std::tuple<buffer_t&, uint64_t, uint32_t, uint32_t>>& buffer_offset_stride_size)
{
	std::vector<D3D12_VERTEX_BUFFER_VIEW> buffer_views;
	for (const auto &vertex_buffer_info : buffer_offset_stride_size)
	{
		D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view = {};

		uint64_t offset;
		std::tie(std::ignore, offset, vertex_buffer_view.StrideInBytes, vertex_buffer_view.SizeInBytes) = vertex_buffer_info;
		vertex_buffer_view.BufferLocation = std::get<0>(vertex_buffer_info)->GetGPUVirtualAddress() + offset;
		buffer_views.push_back(vertex_buffer_view);
	}
	commandlist->IASetVertexBuffers(first_bind, static_cast<uint32_t>(buffer_views.size()), buffer_views.data());

}

void set_graphic_pipeline(command_list_t& command_list, pipeline_state_t pipeline)
{
	command_list->SetPipelineState(pipeline.Get());
}

void set_compute_pipeline(command_list_t& command_list, compute_pipeline_state_t& pipeline)
{
	command_list->SetPipelineState(pipeline);
}

void submit_executable_command_list(command_queue_t& command_queue, command_list_t& command_list)
{
	command_queue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(&command_list.object));
}

void draw_indexed(command_list_t& command_list, uint32_t index_count, uint32_t instance_count, uint32_t base_index, int32_t base_vertex, uint32_t base_instance)
{
	command_list->DrawIndexedInstanced(index_count, instance_count, base_index, base_vertex, base_instance);
}

void draw_non_indexed(command_list_t& command_list, uint32_t vertex_count, uint32_t instance_count, int32_t base_vertex, uint32_t base_instance)
{
	command_list->DrawInstanced(vertex_count, instance_count, base_vertex, base_instance);
}

void dispatch(command_list_t& command_list, uint32_t x, uint32_t y, uint32_t z)
{
	command_list->Dispatch(x, y, z);
}

void copy_buffer(command_list_t& command_list, buffer_t& src, uint64_t src_offset, buffer_t& dst, uint64_t dst_offset, uint64_t size)
{
	command_list->CopyBufferRegion(src, src_offset, dst, dst_offset, size);
}

uint32_t get_next_backbuffer_id(device_t&, swap_chain_t& chain)
{
	return chain->GetCurrentBackBufferIndex();
}

allocated_descriptor_set allocate_descriptor_set_from_cbv_srv_uav_heap(device_t& dev, descriptor_storage_t& heap, uint32_t starting_index, const std::vector<descriptor_set_layout*> &)
{
	return allocated_descriptor_set(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(heap->GetCPUDescriptorHandleForHeapStart())
			.Offset(starting_index, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->GetGPUDescriptorHandleForHeapStart())
			.Offset(starting_index, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
		);
}

allocated_descriptor_set allocate_descriptor_set_from_sampler_heap(device_t& dev, descriptor_storage_t& heap, uint32_t starting_index, const std::vector<descriptor_set_layout*> &)
{
	return allocated_descriptor_set(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(heap->GetCPUDescriptorHandleForHeapStart())
		.Offset(starting_index, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->GetGPUDescriptorHandleForHeapStart())
		.Offset(starting_index, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER))
	);
}

void bind_graphic_descriptor(command_list_t& cmd_list, uint32_t bindpoint, const allocated_descriptor_set& descriptor_set, pipeline_layout_t)
{
	cmd_list->SetGraphicsRootDescriptorTable(bindpoint, descriptor_set);
}

void bind_compute_descriptor(command_list_t& cmd_list, uint32_t bindpoint, const allocated_descriptor_set& descriptor_set, pipeline_layout_t sig)
{
	cmd_list->SetComputeRootDescriptorTable(bindpoint, descriptor_set);
}

void wait_for_command_queue_idle(device_t& dev, command_queue_t& command_queue)
{
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	CHECK_HRESULT(dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())));
	HANDLE completion_event = CreateEvent(0, FALSE, FALSE, 0);
	fence->SetEventOnCompletion(1, completion_event);
	command_queue->Signal(fence.Get(), 1);
	WaitForSingleObject(completion_event, INFINITE);
	CloseHandle(completion_event);
}

void present(device_t&, command_queue_t&, swap_chain_t& chain, uint32_t backbuffer_index)
{
	CHECK_HRESULT(chain->Present(1, 0));
}

d3d12_framebuffer_t::d3d12_framebuffer_t(device_t& dev, const std::vector<std::tuple<image_t&, irr::video::ECOLOR_FORMAT>> &render_targets, const std::tuple<image_t&, irr::video::ECOLOR_FORMAT> &depth_stencil_texture)
	: NumRTT(static_cast<uint32_t>(render_targets.size())), hasDepthStencil(true)
{
	D3D12_DESCRIPTOR_HEAP_DESC rtt_desc = {};
	rtt_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtt_desc.NumDescriptors = NumRTT;
	CHECK_HRESULT(dev->CreateDescriptorHeap(&rtt_desc, IID_PPV_ARGS(rtt_heap.GetAddressOf())));

	uint32_t idx = 0;
	for (const auto &rtt : render_targets)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rttvd = {};
		rttvd.Format = get_dxgi_format(std::get<1>(rtt));
		rttvd.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rttvd.Texture2D.MipSlice = 0;
		rttvd.Texture2D.PlaneSlice = 0;
		dev->CreateRenderTargetView(std::get<0>(rtt), &rttvd, CD3DX12_CPU_DESCRIPTOR_HANDLE(rtt_heap->GetCPUDescriptorHandleForHeapStart()).Offset(idx++, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)));
	}

	D3D12_DESCRIPTOR_HEAP_DESC dsv_desc = {};
	dsv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsv_desc.NumDescriptors = 1;
	CHECK_HRESULT(dev->CreateDescriptorHeap(&dsv_desc, IID_PPV_ARGS(dsv_heap.GetAddressOf())));
	D3D12_DEPTH_STENCIL_VIEW_DESC DSVDescription = {};
	DSVDescription.Format = get_dxgi_format(std::get<1>(depth_stencil_texture));
	DSVDescription.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	DSVDescription.Texture2D.MipSlice = 0;
	dev->CreateDepthStencilView(std::get<0>(depth_stencil_texture), &DSVDescription, dsv_heap->GetCPUDescriptorHandleForHeapStart());
}

d3d12_framebuffer_t::~d3d12_framebuffer_t()
{

}

std::vector<std::unique_ptr<image_t>> get_image_view_from_swap_chain(device_t&, swap_chain_t& chain)
{
	std::vector<std::unique_ptr<image_t>> result;
	for (int i = 0; i < 2; i++)
	{
		ID3D12Resource* back_buffer;
		CHECK_HRESULT(chain->GetBuffer(i, IID_PPV_ARGS(&back_buffer)));
		back_buffer->SetName(L"BackBuffer");
		result.push_back(std::make_unique<image_t>(back_buffer));
	}
	return result;
}
