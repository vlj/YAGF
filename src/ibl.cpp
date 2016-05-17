// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#include <Scene/IBL.h>
#include <Maths/matrix4.h>
#include <cmath>
#include <set>
#include <d3dcompiler.h>
#include "..\include\Scene\IBL.h"

namespace
{
	constexpr auto object_descriptor_set_type = descriptor_set({
		range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 0, 1),
		range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 1, 1),
		range_of_descriptors(RESOURCE_VIEW::UAV_BUFFER, 2, 1) },
		shader_stage::all);


	constexpr auto face_set_type = descriptor_set({
		range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 0, 1),
		range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 1, 1) },
		shader_stage::all);

	constexpr auto mipmap_set_type = descriptor_set({
		range_of_descriptors(RESOURCE_VIEW::TEXEL_BUFFER, 2, 1),
		range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 5, 1) },
		shader_stage::all);

	constexpr auto uav_set_type = descriptor_set({
		range_of_descriptors(RESOURCE_VIEW::UAV_IMAGE, 3, 1) },
		shader_stage::all);

	constexpr auto dfg_input = descriptor_set({
		range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 0, 1),
		range_of_descriptors(RESOURCE_VIEW::TEXEL_BUFFER, 1, 1),
		range_of_descriptors(RESOURCE_VIEW::UAV_IMAGE, 2, 1) },
		shader_stage::all);

	constexpr auto sampler_descriptor_set_type = descriptor_set({
		range_of_descriptors(RESOURCE_VIEW::SAMPLER, 4, 1) },
		shader_stage::fragment_shader);

	std::unique_ptr<compute_pipeline_state_t> get_compute_sh_pipeline_state(device_t& dev, pipeline_layout_t pipeline_layout)
	{
#ifdef D3D12
		ID3D12PipelineState* result;
		Microsoft::WRL::ComPtr<ID3DBlob> blob;
		CHECK_HRESULT(D3DReadFileToBlob(L"computesh.cso", blob.GetAddressOf()));

		D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_desc{};
		pipeline_desc.CS.BytecodeLength = blob->GetBufferSize();
		pipeline_desc.CS.pShaderBytecode = blob->GetBufferPointer();
		pipeline_desc.pRootSignature = pipeline_layout.Get();

		CHECK_HRESULT(dev->CreateComputePipelineState(&pipeline_desc, IID_PPV_ARGS(&result)));
		return std::make_unique<compute_pipeline_state_t>(result);
#else
		vulkan_wrapper::shader_module module(dev, "..\\..\\..\\computesh.spv");
		VkPipelineShaderStageCreateInfo shader_stages{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_COMPUTE_BIT, module.object, "main", nullptr };
		return std::make_unique<compute_pipeline_state_t>(dev, shader_stages, pipeline_layout->object, VkPipeline(VK_NULL_HANDLE), -1);
#endif // D3D12
	}

	std::unique_ptr<compute_pipeline_state_t> ImportanceSamplingForSpecularCubemap(device_t& dev, pipeline_layout_t pipeline_layout)
	{
#ifdef D3D12
		ID3D12PipelineState* result;
		Microsoft::WRL::ComPtr<ID3DBlob> blob;
		CHECK_HRESULT(D3DReadFileToBlob(L"importance_sampling_specular.cso", blob.GetAddressOf()));

		D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_desc{};
		pipeline_desc.CS.BytecodeLength = blob->GetBufferSize();
		pipeline_desc.CS.pShaderBytecode = blob->GetBufferPointer();
		pipeline_desc.pRootSignature = pipeline_layout.Get();

		CHECK_HRESULT(dev->CreateComputePipelineState(&pipeline_desc, IID_PPV_ARGS(&result)));
		return std::make_unique<compute_pipeline_state_t>(result);
#else
		vulkan_wrapper::shader_module module(dev, "..\\..\\..\\importance_sampling_specular.spv");
		VkPipelineShaderStageCreateInfo shader_stages{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_COMPUTE_BIT, module.object, "main", nullptr };
		return std::make_unique<compute_pipeline_state_t>(dev, shader_stages, pipeline_layout->object, VkPipeline(VK_NULL_HANDLE), -1);
#endif // D3D12
	}

	std::unique_ptr<compute_pipeline_state_t> dfg_building_pso(device_t& dev, pipeline_layout_t pipeline_layout)
	{
#ifdef D3D12
		ID3D12PipelineState* result;
		Microsoft::WRL::ComPtr<ID3DBlob> blob;
		CHECK_HRESULT(D3DReadFileToBlob(L"dfg.cso", blob.GetAddressOf()));

		D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_desc{};
		pipeline_desc.CS.BytecodeLength = blob->GetBufferSize();
		pipeline_desc.CS.pShaderBytecode = blob->GetBufferPointer();
		pipeline_desc.pRootSignature = pipeline_layout.Get();

		CHECK_HRESULT(dev->CreateComputePipelineState(&pipeline_desc, IID_PPV_ARGS(&result)));
		return std::make_unique<compute_pipeline_state_t>(result);
#else
		vulkan_wrapper::shader_module module(dev, "..\\..\\..\\dfg.spv");
		VkPipelineShaderStageCreateInfo shader_stages{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_COMPUTE_BIT, module.object, "main", nullptr };
		return std::make_unique<compute_pipeline_state_t>(dev, shader_stages, pipeline_layout->object, VkPipeline(VK_NULL_HANDLE), -1);
#endif // D3D12
	}

	irr::core::matrix4 getPermutationMatrix(size_t indexX, float valX, size_t indexY, float valY, size_t indexZ, float valZ)
	{
		irr::core::matrix4 resultMat;
		float *M = resultMat.pointer();
		memset(M, 0, 16 * sizeof(float));
		assert(indexX < 4);
		assert(indexY < 4);
		assert(indexZ < 4);
		M[indexX] = valX;
		M[4 + indexY] = valY;
		M[8 + indexZ] = valZ;
		return resultMat;
	}

	struct PermutationMatrix
	{
		float Matrix[16];
	};

	// From http://http.developer.nvidia.com/GPUGems3/gpugems3_ch20.html
	/**
	* Returns the n-th set from the 2 dimension Hammersley sequence.
	* The 2 dimension Hammersley seq is a pseudo random uniform distribution
	*  between 0 and 1 for 2 components.
	* We use the natural indexation on the set to avoid storing the whole set.
	* \param i index of the pair
	* \param size of the set. */
	std::pair<float, float> HammersleySequence(int n, int samples)
	{
		float InvertedBinaryRepresentation = 0.;
		for (size_t i = 0; i < 32; i++)
		{
			InvertedBinaryRepresentation += ((n >> i) & 0x1) * powf(.5, (float)(i + 1.));
		}
		return std::make_pair(float(n) / float(samples), InvertedBinaryRepresentation);
	}

	struct per_level_importance_sampling_data
	{
		float size;
		float alpha;
	};
}

ibl_utility::ibl_utility(device_t &dev)
{
#ifdef D3D12
	compute_sh_sig = get_pipeline_layout_from_desc(dev, { object_descriptor_set_type, sampler_descriptor_set_type });
#else
	object_set = get_object_descriptor_set(dev, object_descriptor_set_type);
	sampler_set = get_object_descriptor_set(dev, sampler_descriptor_set_type);
	compute_sh_sig = std::make_shared<vulkan_wrapper::pipeline_layout>(dev, 0, std::vector<VkDescriptorSetLayout>{ object_set->object, sampler_set->object}, std::vector<VkPushConstantRange>());
#endif
	compute_sh_pso = get_compute_sh_pipeline_state(dev, compute_sh_sig);

#ifdef D3D12
	importance_sampling_sig = get_pipeline_layout_from_desc(dev, { face_set_type, mipmap_set_type, uav_set_type, sampler_descriptor_set_type });
#else
	face_set = get_object_descriptor_set(dev, face_set_type);
	mipmap_set = get_object_descriptor_set(dev, mipmap_set_type);
	uav_set = get_object_descriptor_set(dev, uav_set_type);
	sampler_set = get_object_descriptor_set(dev, sampler_descriptor_set_type);
	importance_sampling_sig = std::make_shared<vulkan_wrapper::pipeline_layout>(dev, 0,
		std::vector<VkDescriptorSetLayout>{ face_set->object, mipmap_set->object, uav_set->object, sampler_set->object }, std::vector<VkPushConstantRange>());
#endif
	importance_sampling = ImportanceSamplingForSpecularCubemap(dev, importance_sampling_sig);

#ifdef D3D12
	dfg_building_sig = get_pipeline_layout_from_desc(dev, { dfg_input });
#else
	dfg_set = get_object_descriptor_set(dev, dfg_input);
	dfg_building_sig = std::make_shared<vulkan_wrapper::pipeline_layout>(dev, 0, std::vector<VkDescriptorSetLayout>{ dfg_set->object }, std::vector<VkPushConstantRange>());
#endif
	pso = dfg_building_pso(dev, dfg_building_sig);

	srv_cbv_uav_heap = create_descriptor_storage(dev, 100, { { RESOURCE_VIEW::CONSTANTS_BUFFER, 20 },{ RESOURCE_VIEW::SHADER_RESOURCE, 20 },{ RESOURCE_VIEW::UAV_BUFFER, 1 },{ RESOURCE_VIEW::TEXEL_BUFFER, 10 }, { RESOURCE_VIEW::UAV_IMAGE, 50 } });
	sampler_heap = create_descriptor_storage(dev, 10, { { RESOURCE_VIEW::SAMPLER, 1 } });

	sampler_descriptors = allocate_descriptor_set_from_cbv_srv_uav_heap(dev, *sampler_heap, 0, { sampler_set.get() });

	anisotropic_sampler = create_sampler(dev, SAMPLER_TYPE::ANISOTROPIC);
	set_sampler(dev, sampler_descriptors, 0, 4, *anisotropic_sampler);

	compute_sh_cbuf = create_buffer(dev, sizeof(int), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, none);

	irr::core::matrix4 M[6] = {
		getPermutationMatrix(2, -1., 1, -1., 0, 1.),
		getPermutationMatrix(2, 1., 1, -1., 0, -1.),
		getPermutationMatrix(0, 1., 2, 1., 1, 1.),
		getPermutationMatrix(0, 1., 2, -1., 1, -1.),
		getPermutationMatrix(0, 1., 1, -1., 2, 1.),
		getPermutationMatrix(0, -1., 1, -1., 2, -1.),
	};

	for (unsigned i = 0; i < 6; i++)
	{
		permutation_matrix[i] = create_buffer(dev, sizeof(PermutationMatrix), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, none);
		memcpy(map_buffer(dev, *permutation_matrix[i]), M[i].pointer(), 16 * sizeof(float));
		unmap_buffer(dev, *permutation_matrix[i]);
	}

	hammersley_sequence_buffer = create_buffer(dev, 2048 * sizeof(float), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, usage_texel_buffer);
	float *tmp = reinterpret_cast<float*>(map_buffer(dev, *hammersley_sequence_buffer));
	for (unsigned j = 0; j < 1024; j++)
	{
		std::pair<float, float> sample = HammersleySequence(j, 1024);
		tmp[2 * j] = sample.first;
		tmp[2 * j + 1] = sample.second;
	}
	unmap_buffer(dev, *hammersley_sequence_buffer);
#ifndef D3D12
	hammersley_sequence_buffer_view = std::make_unique<vulkan_wrapper::buffer_view>(dev, *hammersley_sequence_buffer, VK_FORMAT_R32G32_SFLOAT, 0, 2048 * sizeof(float));
#endif

	for (unsigned i = 0; i < 8; i++)
	{
		per_level_cbuffer[i] = create_buffer(dev, sizeof(per_level_importance_sampling_data), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, none);
		per_level_importance_sampling_data* tmp = (per_level_importance_sampling_data*)map_buffer(dev, *per_level_cbuffer[i]);
		float viewportSize = float(1 << (8 - i));
		tmp->size = viewportSize;
		tmp->alpha = .05f + .95f * i / 8.f;
		unmap_buffer(dev, *per_level_cbuffer[i].get());
	}
}

struct SHCoefficients
{
	float Red[9];
	float Green[9];
	float Blue[9];
};

allocated_descriptor_set ibl_utility::get_compute_sh_descriptor(device_t &dev, buffer_t &constant_buffer, image_view_t& probe_view, buffer_t& sh_buffer)
{
	allocated_descriptor_set input_descriptors = allocate_descriptor_set_from_cbv_srv_uav_heap(dev, *srv_cbv_uav_heap, 0, { object_set.get() });

	set_image_view(dev, input_descriptors, 1, 1, probe_view);
	set_constant_buffer_view(dev, input_descriptors, 0, 0, constant_buffer, sizeof(int));
#ifdef D3D12
	create_buffer_uav_view(dev, *srv_cbv_uav_heap, 2, sh_buffer, sizeof(SH));
#else
	util::update_descriptor_sets(dev,
	{
		structures::write_descriptor_set(input_descriptors, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			{ structures::descriptor_buffer_info(sh_buffer, 0, sizeof(SH)) }, 2)
	});
#endif
	return input_descriptors;
}

allocated_descriptor_set ibl_utility::get_dfg_input_descriptor_set(device_t & dev, buffer_t& constant_buffer, image_view_t &DFG_LUT_view)
{
	allocated_descriptor_set dfg_input_descriptor_set = allocate_descriptor_set_from_cbv_srv_uav_heap(dev, *srv_cbv_uav_heap, 0, { dfg_set.get() });
	set_constant_buffer_view(dev, dfg_input_descriptor_set, 0, 0, constant_buffer, sizeof(float));

#if D3D12
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
	desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

	dev->CreateUnorderedAccessView(std::get<1>(DFG_LUT_view), nullptr, &desc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(dfg_input_descriptor_set)
		.Offset(2, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));
#else
	util::update_descriptor_sets(dev, {
		structures::write_descriptor_set(dfg_input_descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			{ structures::descriptor_image_info(DFG_LUT_view) }, 2)
	});
#endif

	return dfg_input_descriptor_set;
}

std::unique_ptr<buffer_t> ibl_utility::computeSphericalHarmonics(device_t& dev, command_list_t& cmd_list, image_view_t& probe_view, size_t edge_size)
{
	void* tmp = map_buffer(dev, *compute_sh_cbuf);
	float cube_size = static_cast<float>(edge_size) / 10.f;
	memcpy(tmp, &cube_size, sizeof(int));
	unmap_buffer(dev, *compute_sh_cbuf);

	std::unique_ptr<buffer_t> sh_buffer = create_buffer(dev, sizeof(SH), irr::video::E_MEMORY_POOL::EMP_GPU_LOCAL, usage_uav);

#ifdef D3D12
	cmd_list->SetComputeRootSignature(compute_sh_sig.Get());
	std::array<ID3D12DescriptorHeap*, 2> heaps = { srv_cbv_uav_heap->object, sampler_heap->object };
	cmd_list->SetDescriptorHeaps(2, heaps.data());
	cmd_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(sh_buffer->object, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
//	command_list->object->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(sh_buffer->object, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
//	command_list->object->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(sh_buffer->object));
#endif
	set_compute_pipeline(cmd_list, *compute_sh_pso);
	bind_compute_descriptor(cmd_list, 0, get_compute_sh_descriptor(dev, *compute_sh_cbuf, probe_view, *sh_buffer), compute_sh_sig);
	bind_compute_descriptor(cmd_list, 1, sampler_descriptors, compute_sh_sig);

	dispatch(cmd_list, 1, 1, 1);
	// for debug
	//	std::unique_ptr<buffer_t> sh_buffer_readback = create_buffer(dev, sizeof(SH), irr::video::E_MEMORY_POOL::EMP_CPU_READABLE, usage_transfer_dst);
	//	copy_buffer(command_list.get(), sh_buffer.get(), 0, sh_buffer_readback.get(), 0, sizeof(SH));
/*	SHCoefficients Result;
	float* Shval = (float*)map_buffer(dev, sh_buffer_readback.get());
	memcpy(Result.Blue, Shval, 9 * sizeof(float));
	memcpy(Result.Green, &Shval[9], 9 * sizeof(float));
	memcpy(Result.Red, &Shval[18], 9 * sizeof(float));*/

	return std::move(sh_buffer);
}

std::unique_ptr<image_t> ibl_utility::generateSpecularCubemap(device_t& dev, command_list_t& cmd_list, image_view_t& probe_view)
{
	size_t cubemap_size = 256;
	std::array<allocated_descriptor_set, 6> permutation_matrix_descriptors;
	for (unsigned i = 0; i < 6; i++)
	{
		permutation_matrix_descriptors[i] = allocate_descriptor_set_from_cbv_srv_uav_heap(dev, *srv_cbv_uav_heap, 2 * i, { face_set.get() });
		set_image_view(dev, permutation_matrix_descriptors[i], 0, 0, probe_view);
		set_constant_buffer_view(dev, permutation_matrix_descriptors[i], 1, 1, *permutation_matrix[i], sizeof(PermutationMatrix));
	}

	std::array<allocated_descriptor_set, 8> sample_buffer_descriptors;
	for (unsigned i = 0; i < 8; i++)
	{
		sample_buffer_descriptors[i] = allocate_descriptor_set_from_cbv_srv_uav_heap(dev, *srv_cbv_uav_heap, 2 * i + 12, { mipmap_set.get() });
		set_constant_buffer_view(dev, sample_buffer_descriptors[i], 1, 5, *per_level_cbuffer[i], sizeof(float));
#ifdef D3D12
		D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
		srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srv.Buffer.FirstElement = 0;
		srv.Buffer.NumElements = 1024;
		srv.Buffer.StructureByteStride = 2 * sizeof(float);
		srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		dev->CreateShaderResourceView(sample_location_buffer[i]->object, &srv, sample_buffer_descriptors[i]);
#else

		util::update_descriptor_sets(dev,
		{
			structures::write_descriptor_set(sample_buffer_descriptors[i], VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
				{*hammersley_sequence_buffer_view}, 2),
		});
#endif
	}

	std::unique_ptr<image_t> result = create_image(dev, irr::video::ECF_R16G16B16A16F, 256, 256, 8, 6, usage_cube | usage_sampled | usage_uav, nullptr);
	set_compute_pipeline(cmd_list, *importance_sampling);
#ifdef D3D12
	cmd_list->SetComputeRootSignature(importance_sampling_sig.Get());
	std::array<ID3D12DescriptorHeap*, 2> heaps{ srv_cbv_uav_heap->object, sampler_heap->object };
	cmd_list->SetDescriptorHeaps(2, heaps.data());
#endif

//	bind_compute_descriptor(command_list.get(), 0, image_descriptors, importance_sampling_sig);
	bind_compute_descriptor(cmd_list, 3, sampler_descriptors, importance_sampling_sig);

	for (unsigned level = 0; level < 8; level++)
	{
		bind_compute_descriptor(cmd_list, 1, sample_buffer_descriptors[level], importance_sampling_sig);
		for (unsigned face = 0; face < 6; face++)
		{
			allocated_descriptor_set level_face_descriptor = allocate_descriptor_set_from_cbv_srv_uav_heap(dev, *srv_cbv_uav_heap, face + level * 6 + 28, { uav_set.get() });
#ifdef D3D12
			D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
			desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.MipSlice = level;
			desc.Texture2DArray.ArraySize = 1;
			desc.Texture2DArray.FirstArraySlice = face;
			desc.Texture2DArray.PlaneSlice = 0;
			desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

			dev->CreateUnorderedAccessView(result->object, nullptr, &desc,
				CD3DX12_CPU_DESCRIPTOR_HANDLE(level_face_descriptor));
#else
			uav_views[face + level * 6] = create_image_view(dev, *result, VK_FORMAT_R16G16B16A16_SFLOAT, structures::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT, level, 1, face, 1), VK_IMAGE_VIEW_TYPE_CUBE);
			util::update_descriptor_sets(dev, {
				structures::write_descriptor_set(level_face_descriptor, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				{ structures::descriptor_image_info(uav_views[face + level * 6]->object, VK_IMAGE_LAYOUT_GENERAL) }, 3)
			});
#endif

			bind_compute_descriptor(cmd_list, 0, permutation_matrix_descriptors[face], importance_sampling_sig);
			bind_compute_descriptor(cmd_list, 2, level_face_descriptor, importance_sampling_sig);

			dispatch(cmd_list, 256 >> level, 256 >> level, 1);
		}
	}

	return result;
}

/** Generate the Look Up Table for the DFG texture.
	DFG Texture is used to compute diffuse and specular response from environmental lighting. */
std::unique_ptr<image_t> ibl_utility::getDFGLUT(device_t& dev, command_queue_t& cmdqueue, uint32_t DFG_LUT_size)
{
	std::unique_ptr<image_t> DFG_LUT_texture = create_image(dev, irr::video::ECF_R32G32B32A32F, DFG_LUT_size, DFG_LUT_size, 1, 1, usage_sampled | usage_uav, nullptr);
	std::unique_ptr<command_list_storage_t> command_storage = create_command_storage(dev);
	std::unique_ptr<command_list_t> command_list = create_command_list(dev, *command_storage);

	std::unique_ptr<buffer_t> cbuf = create_buffer(dev, sizeof(float), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, none);
	void* tmp = map_buffer(dev, *cbuf);
	float sz = DFG_LUT_size;
	memcpy(tmp, &sz, sizeof(float));
	unmap_buffer(dev, *cbuf);
	std::unique_ptr<buffer_t> hamersley_seq_buf = create_buffer(dev, 2048 * sizeof(float), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, usage_texel_buffer);
	float* hamerleybuf = (float*)map_buffer(dev, *hamersley_seq_buf);
	for (unsigned j = 0; j < 1024; j++)
	{
		std::pair<float, float> sample = HammersleySequence(j, 1024);
		hamerleybuf[2 * j] = sample.first;
		hamerleybuf[2 * j + 1] = sample.second;
	}
	unmap_buffer(dev, *hamersley_seq_buf);
	std::unique_ptr<image_view_t> texture_view = create_image_view(dev, *DFG_LUT_texture, irr::video::ECF_R32G32B32A32F, 1, 1, irr::video::E_TEXTURE_TYPE::ETT_2D);

	allocated_descriptor_set dfg_input_descriptor_set = get_dfg_input_descriptor_set(dev, *cbuf, *texture_view);
#if D3D12
	D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
	srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srv.Buffer.FirstElement = 0;
	srv.Buffer.NumElements = 1024;
	srv.Buffer.StructureByteStride = 2 * sizeof(float);
	srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	dev->CreateShaderResourceView(*hamersley_seq_buf, &srv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(dfg_input_descriptor_set)
		.Offset(1, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));
#else
	std::unique_ptr<vulkan_wrapper::buffer_view> buffer_view = std::make_unique<vulkan_wrapper::buffer_view>(dev, *hamersley_seq_buf, VK_FORMAT_R32G32_SFLOAT, 0, 2048 * sizeof(float));

	util::update_descriptor_sets(dev, {
		structures::write_descriptor_set(dfg_input_descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
		{ *buffer_view }, 1),
	});
#endif

	start_command_list_recording(*command_list, *command_storage);
	set_compute_pipeline(*command_list, *pso);
#ifdef D3D12
	command_list->object->SetComputeRootSignature(dfg_building_sig.Get());
	std::array<ID3D12DescriptorHeap*, 1> heaps{ *srv_cbv_uav_heap };
	command_list->object->SetDescriptorHeaps(1, heaps.data());
#endif // D3D12
	bind_compute_descriptor(*command_list, 0, dfg_input_descriptor_set, dfg_building_sig);

	dispatch(*command_list, DFG_LUT_size, DFG_LUT_size, 1);

	make_command_list_executable(*command_list);
	submit_executable_command_list(cmdqueue, *command_list);
	wait_for_command_queue_idle(dev, cmdqueue);

	return DFG_LUT_texture;
}
