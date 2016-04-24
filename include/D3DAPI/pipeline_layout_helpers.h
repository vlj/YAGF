// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once
#include "../API/d3dapi.h"
#include <d3d12.h>
#include <d3dx12.h>


#define CHECK_HRESULT(cmd) {HRESULT hr = cmd; if (hr != 0) throw;}

inline D3D12_DESCRIPTOR_RANGE_TYPE get_range_type(RESOURCE_VIEW type)
{
	switch (type)
	{
	case RESOURCE_VIEW::CONSTANTS_BUFFER: return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	case RESOURCE_VIEW::INPUT_ATTACHMENT: return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	case RESOURCE_VIEW::SHADER_RESOURCE: return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	case RESOURCE_VIEW::SAMPLER: return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	}
	throw;
}

inline pipeline_layout_t get_pipeline_layout_from_desc(device_t dev, const std::vector<descriptor_set_> desc)
{
	pipeline_layout_t result;

	std::vector<std::vector<D3D12_DESCRIPTOR_RANGE> > all_descriptor_range_storage;
	all_descriptor_range_storage.reserve(desc.size());
	std::vector<D3D12_ROOT_PARAMETER> root_parameters;
	root_parameters.reserve(desc.size());
	for (const auto &root_parameter_desc : desc)
	{
		std::vector<D3D12_DESCRIPTOR_RANGE> descriptor_range_storage;
		descriptor_range_storage.reserve(root_parameter_desc.count);
		uint32_t offset_from_start = 0;
		for (uint32_t i = 0; i < root_parameter_desc.count; i++)
		{
			const range_of_descriptors &rod = root_parameter_desc.descriptors_ranges[i];
			D3D12_DESCRIPTOR_RANGE range{};
			range.RegisterSpace = rod.bind_point;
			range.NumDescriptors = rod.count;
			range.OffsetInDescriptorsFromTableStart = offset_from_start;
			range.RangeType = get_range_type(rod.range_type);
			descriptor_range_storage.emplace_back(range);
			offset_from_start += rod.count;
		}
		all_descriptor_range_storage.emplace_back(descriptor_range_storage);
		CD3DX12_ROOT_PARAMETER rp;
		rp.InitAsDescriptorTable(static_cast<uint32_t>(all_descriptor_range_storage.back().size()), all_descriptor_range_storage.back().data(), D3D12_SHADER_VISIBILITY_ALL);
		root_parameters.push_back(rp);
	}
	D3D12_ROOT_SIGNATURE_DESC rs_desc{};
	rs_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rs_desc.NumParameters = static_cast<uint32_t>(root_parameters.size());
	rs_desc.pParameters = root_parameters.data();

	Microsoft::WRL::ComPtr<ID3DBlob> pSerializedRootSig;
	Microsoft::WRL::ComPtr<ID3DBlob> error;
	CHECK_HRESULT(D3D12SerializeRootSignature(&rs_desc, D3D_ROOT_SIGNATURE_VERSION_1, pSerializedRootSig.GetAddressOf(), error.GetAddressOf()));

	CHECK_HRESULT(dev->CreateRootSignature(1,
		pSerializedRootSig->GetBufferPointer(), pSerializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(result.GetAddressOf())));
	return result;
}

