// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#include <Scene/IBL.h>
#include <Maths/matrix4.h>
#include <cmath>
#include <set>
#include <d3dcompiler.h>

namespace
{
	constexpr auto object_descriptor_set_type = descriptor_set({
		range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 0, 1),
		range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 1, 1),
		range_of_descriptors(RESOURCE_VIEW::UAV_BUFFER, 2, 1) },

		shader_stage::all);

	constexpr auto sampler_descriptor_set_type = descriptor_set({
		range_of_descriptors(RESOURCE_VIEW::SAMPLER, 3, 1) },
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

		CHECK_HRESULT(dev->object->CreateComputePipelineState(&pipeline_desc, IID_PPV_ARGS(&result)));
		return std::make_unique<compute_pipeline_state_t>(result);
#else
		vulkan_wrapper::shader_module module(dev, "..\\..\\..\\computesh.spv");
		VkPipelineShaderStageCreateInfo shader_stages{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_COMPUTE_BIT, module.object, "main", nullptr };
		return std::make_unique<compute_pipeline_state_t>(dev, shader_stages, pipeline_layout->object, VkPipeline(VK_NULL_HANDLE), -1);
#endif // D3D12
	}

}

struct SHCoefficients
{
	float Red[9];
	float Green[9];
	float Blue[9];
};


std::unique_ptr<buffer_t> computeSphericalHarmonics(device_t& dev, command_queue_t& cmd_queue, image_t& probe, size_t edge_size)
{
	std::unique_ptr<command_list_storage_t> command_storage = create_command_storage(dev);
	std::unique_ptr<command_list_t> command_list = create_command_list(dev, *command_storage);

	start_command_list_recording(*command_list, *command_storage);
	std::unique_ptr<buffer_t> cbuf = create_buffer(dev, sizeof(int), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, none);
	void* tmp = map_buffer(dev, *cbuf);
	float cube_size = static_cast<float>(edge_size) / 10.f;
	memcpy(tmp, &cube_size, sizeof(int));
	unmap_buffer(dev, *cbuf);
	std::shared_ptr<descriptor_set_layout> object_set;
	std::shared_ptr<descriptor_set_layout> sampler_set;
#ifdef D3D12
	auto compute_sh_sig = get_pipeline_layout_from_desc(dev, { object_descriptor_set_type, sampler_descriptor_set_type });
#else
	object_set = get_object_descriptor_set(dev, object_descriptor_set_type);
	sampler_set = get_object_descriptor_set(dev, sampler_descriptor_set_type);
	auto compute_sh_sig = std::make_shared<vulkan_wrapper::pipeline_layout>(dev, 0, std::vector<VkDescriptorSetLayout>{ object_set->object, sampler_set->object}, std::vector<VkPushConstantRange>());
#endif
	std::unique_ptr<compute_pipeline_state_t> compute_sh_pso = get_compute_sh_pipeline_state(dev, compute_sh_sig);
	std::unique_ptr<descriptor_storage_t> srv_cbv_uav_heap = create_descriptor_storage(dev, 1, { { RESOURCE_VIEW::CONSTANTS_BUFFER, 1 }, { RESOURCE_VIEW::SHADER_RESOURCE, 1}, { RESOURCE_VIEW::UAV_BUFFER, 1} });
	std::unique_ptr<descriptor_storage_t> sampler_heap = create_descriptor_storage(dev, 1, { { RESOURCE_VIEW::SAMPLER, 1 } });

	std::unique_ptr<buffer_t> sh_buffer = create_buffer(dev, sizeof(SH), irr::video::E_MEMORY_POOL::EMP_GPU_LOCAL, usage_uav);
	std::unique_ptr<buffer_t> sh_buffer_readback = create_buffer(dev, sizeof(SH), irr::video::E_MEMORY_POOL::EMP_CPU_READABLE, usage_transfer_dst);
	allocated_descriptor_set input_descriptors = allocate_descriptor_set_from_cbv_srv_uav_heap(dev, *srv_cbv_uav_heap, 0, { object_set.get() });
	allocated_descriptor_set sampler_descriptor = allocate_descriptor_set_from_cbv_srv_uav_heap(dev, *sampler_heap, 0, { sampler_set.get() });
#ifdef D3D12
	create_constant_buffer_view(dev, input_descriptors, 0, cbuf.get(), sizeof(int));
	create_image_view(dev, input_descriptors, 1, probe, 9, irr::video::ECF_BC1_UNORM_SRGB, D3D12_SRV_DIMENSION_TEXTURECUBE);
	create_buffer_uav_view(dev, srv_cbv_uav_heap.get(), 2, sh_buffer.get(), sizeof(SH));
	create_sampler(dev, sampler_descriptor, 0, SAMPLER_TYPE::ANISOTROPIC);

	command_list->object->SetComputeRootSignature(compute_sh_sig.Get());
	std::array<ID3D12DescriptorHeap*, 2> heaps = { srv_cbv_uav_heap->object, sampler_heap->object };
	command_list->object->SetDescriptorHeaps(2, heaps.data());
	command_list->object->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(sh_buffer->object, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
//	command_list->object->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(sh_buffer->object, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
//	command_list->object->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(sh_buffer->object));

#else
	std::unique_ptr<vulkan_wrapper::sampler> sampler = std::make_unique<vulkan_wrapper::sampler>(dev, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, 0.f, true, 16.f);
	std::unique_ptr<vulkan_wrapper::image_view> skybox_view = std::make_unique<vulkan_wrapper::image_view>(dev, probe, VK_IMAGE_VIEW_TYPE_CUBE, VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
		structures::component_mapping(), structures::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT, 0, 9, 0, 6));

	util::update_descriptor_sets(dev,
	{
		structures::write_descriptor_set(sampler_descriptor, VK_DESCRIPTOR_TYPE_SAMPLER,
			{ VkDescriptorImageInfo{ sampler->object, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } }, 3),
		structures::write_descriptor_set(input_descriptors, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			{ structures::descriptor_buffer_info(cbuf->object, 0, sizeof(int)) }, 0),
		structures::write_descriptor_set(input_descriptors, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			{ structures::descriptor_image_info(skybox_view->object) }, 1),
		structures::write_descriptor_set(input_descriptors, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			{ structures::descriptor_buffer_info(sh_buffer->object, 0, sizeof(SH)) }, 2)
	});
#endif
	set_compute_pipeline(*command_list, *compute_sh_pso);
	bind_compute_descriptor(*command_list, 0, input_descriptors, compute_sh_sig);
	bind_compute_descriptor(*command_list, 1, sampler_descriptor, compute_sh_sig);

	dispatch(*command_list, 1, 1, 1);
//	copy_buffer(command_list.get(), sh_buffer.get(), 0, sh_buffer_readback.get(), 0, sizeof(SH));

	make_command_list_executable(*command_list);
	submit_executable_command_list(cmd_queue, *command_list);
	// for debug
	wait_for_command_queue_idle(dev, cmd_queue);
/*	SHCoefficients Result;
	float* Shval = (float*)map_buffer(dev, sh_buffer_readback.get());
	memcpy(Result.Blue, Shval, 9 * sizeof(float));
	memcpy(Result.Green, &Shval[9], 9 * sizeof(float));
	memcpy(Result.Red, &Shval[18], 9 * sizeof(float));*/

	return std::move(sh_buffer);
}

namespace
{
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

/** Returns a pseudo random (theta, phi) generated from a probability density function modeled after Phong function.
\param a pseudo random float pair from a uniform density function between 0 and 1.
\param exponent from the Phong formula. */
std::pair<float, float> ImportanceSamplingPhong(std::pair<float, float> Seeds, float exponent)
{
	return std::make_pair(acosf(powf(Seeds.first, 1.f / (exponent + 1.f))), 2.f * 3.14f * Seeds.second);
}

/** Returns a pseudo random (theta, phi) generated from a probability density function modeled after GGX distribtion function.
From "Real Shading in Unreal Engine 4" paper
\param a pseudo random float pair from a uniform density function between 0 and 1.
\param exponent from the Phong formula. */
std::pair<float, float> ImportanceSamplingGGX(std::pair<float, float> Seeds, float roughness)
{
	float a = roughness * roughness;
	float CosTheta = sqrtf((1.f - Seeds.second) / (1.f + (a * a - 1.f) * Seeds.second));
	return std::make_pair(acosf(CosTheta), 2.f * 3.14f * Seeds.first);
}

/** Returns a pseudo random (theta, phi) generated from a probability density function modeled after cos distribtion function.
\param a pseudo random float pair from a uniform density function between 0 and 1. */
std::pair<float, float> ImportanceSamplingCos(std::pair<float, float> Seeds)
{
	return std::make_pair(acosf(Seeds.first), 2.f * 3.14f * Seeds.second);
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

constexpr auto sampler_set_type = descriptor_set({
	range_of_descriptors(RESOURCE_VIEW::SAMPLER, 4, 1) },
	shader_stage::all);

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

	CHECK_HRESULT(dev->object->CreateComputePipelineState(&pipeline_desc, IID_PPV_ARGS(&result)));
	return std::make_unique<compute_pipeline_state_t>(result);
#else
	vulkan_wrapper::shader_module module(dev, "..\\..\\..\\importance_sampling_specular.spv");
	VkPipelineShaderStageCreateInfo shader_stages{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_COMPUTE_BIT, module.object, "main", nullptr };
	return std::make_unique<compute_pipeline_state_t>(dev, shader_stages, pipeline_layout->object, VkPipeline(VK_NULL_HANDLE), -1);
#endif // D3D12
}
}

std::unique_ptr<image_t> generateSpecularCubemap(device_t& dev, command_queue_t& cmd_queue, image_t& probe)
{
	size_t cubemap_size = 256;

	std::unique_ptr<command_list_storage_t> command_storage = create_command_storage(dev);
	std::unique_ptr<command_list_t> command_list = create_command_list(dev, *command_storage);

	std::shared_ptr<descriptor_set_layout> face_set;
	std::shared_ptr<descriptor_set_layout> mipmap_set;
	std::shared_ptr<descriptor_set_layout> uav_set;
	std::shared_ptr<descriptor_set_layout> sampler_set;

#ifdef D3D12
	pipeline_layout_t importance_sampling_sig = get_pipeline_layout_from_desc(dev, { face_set_type, mipmap_set_type, uav_set_type, sampler_set_type });
#else
	face_set = get_object_descriptor_set(dev, face_set_type);
	mipmap_set = get_object_descriptor_set(dev, mipmap_set_type);
	uav_set = get_object_descriptor_set(dev, uav_set_type);
	sampler_set = get_object_descriptor_set(dev, sampler_set_type);
	pipeline_layout_t importance_sampling_sig = std::make_shared<vulkan_wrapper::pipeline_layout>(dev, 0,
		std::vector<VkDescriptorSetLayout>{ face_set->object, mipmap_set->object, uav_set->object, sampler_set->object }, std::vector<VkPushConstantRange>());
#endif
	std::unique_ptr<compute_pipeline_state_t> importance_sampling = ImportanceSamplingForSpecularCubemap(dev, importance_sampling_sig);
	std::unique_ptr<descriptor_storage_t> input_heap = create_descriptor_storage(dev, 100, { { RESOURCE_VIEW::SHADER_RESOURCE, 8 }, {RESOURCE_VIEW::TEXEL_BUFFER, 8}, { RESOURCE_VIEW::CONSTANTS_BUFFER, 14 },{ RESOURCE_VIEW::UAV_IMAGE, 48 } });
	std::unique_ptr<descriptor_storage_t> sampler_heap = create_descriptor_storage(dev, 1, { { RESOURCE_VIEW::SAMPLER, 1 } });

	allocated_descriptor_set sampler_descriptors = allocate_descriptor_set_from_sampler_heap(dev, *sampler_heap, 0, { sampler_set.get() });

#ifdef D3D12
	create_sampler(dev, sampler_descriptors, 0, SAMPLER_TYPE::TRILINEAR);
#else
	std::unique_ptr<vulkan_wrapper::image_view> probe_view = std::make_unique<vulkan_wrapper::image_view>(dev, probe, VK_IMAGE_VIEW_TYPE_CUBE, VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
		structures::component_mapping(), structures::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT, 0, 9, 0, 6));
	std::unique_ptr<vulkan_wrapper::sampler> sampler = std::make_unique<vulkan_wrapper::sampler>(dev, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, 0.f, true, 16.f);
	util::update_descriptor_sets(dev,
	{
		structures::write_descriptor_set(sampler_descriptors, VK_DESCRIPTOR_TYPE_SAMPLER,
			{ structures::descriptor_sampler_info(sampler->object) }, 4)
	});
#endif

	irr::core::matrix4 M[6] = {
		getPermutationMatrix(2, -1., 1, -1., 0, 1.),
		getPermutationMatrix(2, 1., 1, -1., 0, -1.),
		getPermutationMatrix(0, 1., 2, 1., 1, 1.),
		getPermutationMatrix(0, 1., 2, -1., 1, -1.),
		getPermutationMatrix(0, 1., 1, -1., 2, 1.),
		getPermutationMatrix(0, -1., 1, -1., 2, -1.),
	};

	std::array<std::unique_ptr<buffer_t>, 6> permutation_matrix{};
	std::array<allocated_descriptor_set, 6> permutation_matrix_descriptors;
	for (unsigned i = 0; i < 6; i++)
	{
		permutation_matrix_descriptors[i] = allocate_descriptor_set_from_cbv_srv_uav_heap(dev, *input_heap, 2 * i, { face_set.get() });
		permutation_matrix[i] = create_buffer(dev, sizeof(PermutationMatrix), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, none);
		memcpy(map_buffer(dev, *permutation_matrix[i]), M[i].pointer(), 16 * sizeof(float));
		unmap_buffer(dev, *permutation_matrix[i]);
#ifdef D3D12
		create_constant_buffer_view(dev, permutation_matrix_descriptors[i], 1, permutation_matrix[i].get(), sizeof(PermutationMatrix));
		create_image_view(dev, permutation_matrix_descriptors[i], 0, probe, 1, irr::video::ECF_BC1_UNORM_SRGB, D3D12_SRV_DIMENSION_TEXTURECUBE);
#else
		util::update_descriptor_sets(dev,
		{
			structures::write_descriptor_set(permutation_matrix_descriptors[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				{ structures::descriptor_buffer_info(permutation_matrix[i]->object, 0, 16 * sizeof(float))}, 1),
			structures::write_descriptor_set(permutation_matrix_descriptors[i], VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
				{ structures::descriptor_image_info(probe_view->object) }, 0),
		});
#endif
	}

	std::array<std::unique_ptr<buffer_t>, 8> sample_location_buffer{};
	std::array<std::unique_ptr<buffer_t>, 8> per_level_cbuffer{};
	std::array<allocated_descriptor_set, 8> sample_buffer_descriptors;
#ifndef D3D12
	std::array<std::unique_ptr<vulkan_wrapper::buffer_view>, 8> buffer_views;
#endif
	for (unsigned i = 0; i < 8; i++)
	{
		sample_buffer_descriptors[i] = allocate_descriptor_set_from_cbv_srv_uav_heap(dev, *input_heap, 2 * i + 12, { mipmap_set.get() });
		sample_location_buffer[i] = create_buffer(dev, 2048 * sizeof(float), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, usage_texel_buffer);
		per_level_cbuffer[i] = create_buffer(dev, sizeof(float), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, none);

		float* sz = (float*)map_buffer(dev, *per_level_cbuffer[i]);
		float viewportSize = float(1 << (8 - i));
		*sz = viewportSize;
		unmap_buffer(dev, *per_level_cbuffer[i].get());

		float roughness = .05f + .95f * i / 8.f;
		float *tmp = reinterpret_cast<float*>(map_buffer(dev, *sample_location_buffer[i]));
		for (unsigned j = 0; j < 1024; j++)
		{
			std::pair<float, float> sample = ImportanceSamplingGGX(HammersleySequence(j, 1024), roughness);
			tmp[2 * j] = sample.first;
			tmp[2 * j + 1] = sample.second;
		}
		unmap_buffer(dev, *sample_location_buffer[i]);
#ifdef D3D12
		create_constant_buffer_view(dev, sample_buffer_descriptors[i], 1, per_level_cbuffer[i].get(), sizeof(float));
		D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
		srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srv.Buffer.FirstElement = 0;
		srv.Buffer.NumElements = 1024;
		srv.Buffer.StructureByteStride = 2 * sizeof(float);
		srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		dev->object->CreateShaderResourceView(sample_location_buffer[i]->object, &srv, sample_buffer_descriptors[i]);
#else
		buffer_views[i] = std::make_unique<vulkan_wrapper::buffer_view>(dev, sample_location_buffer[i]->object, VK_FORMAT_R32G32_SFLOAT, 0, 2048 * sizeof(float));
		util::update_descriptor_sets(dev,
		{
			structures::write_descriptor_set(sample_buffer_descriptors[i], VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
				{buffer_views[i]->object}, 2),
			structures::write_descriptor_set(sample_buffer_descriptors[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				{ structures::descriptor_buffer_info(per_level_cbuffer[i]->object, 0, sizeof(float)) }, 5)
		});
#endif
	}

	std::unique_ptr<image_t> result = create_image(dev, irr::video::ECF_R16G16B16A16F, 256, 256, 8, 6, usage_cube | usage_sampled | usage_uav, nullptr);
	std::array<allocated_descriptor_set, 48> level_face_descriptor;
#ifndef D3D12
	std::array<std::unique_ptr<vulkan_wrapper::image_view>, 48> uav_views;
#endif // !D3D12
	for (unsigned level = 0; level < 8; level++)
	{
		for (unsigned face = 0; face < 6; face++)
		{
			level_face_descriptor[face + level * 6] = allocate_descriptor_set_from_cbv_srv_uav_heap(dev, *input_heap, face + level * 6 + 28, {uav_set.get()});
#ifdef D3D12
			D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
			desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.MipSlice = level;
			desc.Texture2DArray.ArraySize = 1;
			desc.Texture2DArray.FirstArraySlice = face;
			desc.Texture2DArray.PlaneSlice = 0;
			desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

			dev->object->CreateUnorderedAccessView(result->object, nullptr, &desc,
				CD3DX12_CPU_DESCRIPTOR_HANDLE(level_face_descriptor[face + 6 * level]));
#else
			uav_views[face + level * 6] = create_image_view(dev, *result, VK_FORMAT_R16G16B16A16_SFLOAT, structures::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT, level, 1, face, 1), VK_IMAGE_VIEW_TYPE_CUBE);
			util::update_descriptor_sets(dev, {
				structures::write_descriptor_set(level_face_descriptor[face + level * 6], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				{ structures::descriptor_image_info(uav_views[face + level * 6]->object, VK_IMAGE_LAYOUT_GENERAL)}, 3)
			});
#endif
		}
	}

	start_command_list_recording(*command_list, *command_storage);
	set_compute_pipeline(*command_list, *importance_sampling);
#ifdef D3D12
	command_list->object->SetComputeRootSignature(importance_sampling_sig.Get());
	std::array<ID3D12DescriptorHeap*, 2> heaps{ input_heap->object, sampler_heap->object };
	command_list->object->SetDescriptorHeaps(2, heaps.data());
#endif

//	bind_compute_descriptor(command_list.get(), 0, image_descriptors, importance_sampling_sig);
	bind_compute_descriptor(*command_list, 3, sampler_descriptors, importance_sampling_sig);

	for (unsigned level = 0; level < 8; level++)
	{
		bind_compute_descriptor(*command_list, 1, sample_buffer_descriptors[level], importance_sampling_sig);
		for (unsigned face = 0; face < 6; face++)
		{
			bind_compute_descriptor(*command_list, 0, permutation_matrix_descriptors[face], importance_sampling_sig);
			bind_compute_descriptor(*command_list, 2, level_face_descriptor[face + 6 * level], importance_sampling_sig);

			dispatch(*command_list, 256 >> level, 256 >> level, 1);
		}
	}
	make_command_list_executable(*command_list);
	submit_executable_command_list(cmd_queue, *command_list);
	wait_for_command_queue_idle(dev, cmd_queue);

	return result;
}

#if 0
static float G1_Schlick(const irr::core::vector3df &V, const irr::core::vector3df &normal, float k)
{
	float NdotV = V.dotProduct(normal);
	NdotV = NdotV > 0.f ? NdotV : 0.f;
	NdotV = NdotV < 1.f ? NdotV : 1.f;
	return 1.f / (NdotV * (1.f - k) + k);
}

float G_Smith(const irr::core::vector3df &lightdir, const irr::core::vector3df &viewdir, const irr::core::vector3df &normal, float roughness)
{
	float k = (roughness + 1.f) * (roughness + 1.f) / 8.f;
	return G1_Schlick(lightdir, normal, k) * G1_Schlick(viewdir, normal, k);
}

static
std::pair<float, float> getSpecularDFG(float roughness, float NdotV)
{
	// We assume a local referential where N points in Y direction
	irr::core::vector3df V(sqrtf(1.f - NdotV * NdotV), NdotV, 0.f);

	float DFG1 = 0., DFG2 = 0.;
	for (unsigned sample = 0; sample < 1024; sample++)
	{
		std::pair<float, float> ThetaPhi = ImportanceSamplingGGX(HammersleySequence(sample, 1024), roughness);
		float Theta = ThetaPhi.first, Phi = ThetaPhi.second;
		irr::core::vector3df H(sinf(Theta) * cosf(Phi), cosf(Theta), sinf(Theta) * sinf(Phi));
		irr::core::vector3df L = 2 * H.dotProduct(V) * H - V;
		float NdotL = L.Y;

		float NdotH = H.Y > 0. ? H.Y : 0.;
		if (NdotL > 0.)
		{
			float VdotH = V.dotProduct(H);
			VdotH = VdotH > 0.f ? VdotH : 0.f;
			VdotH = VdotH < 1.f ? VdotH : 1.f;
			float Fc = powf(1.f - VdotH, 5.f);
			float G = G_Smith(L, V, irr::core::vector3df(0.f, 1.f, 0.f), roughness);
			DFG1 += (1.f - Fc) * G * VdotH;
			DFG2 += Fc * G * VdotH;
		}
	}
	return std::make_pair(DFG1 / 1024, DFG2 / 1024);
}

static
float getDiffuseDFG(float roughness, float NdotV)
{
	// We assume a local referential where N points in Y direction
	irr::core::vector3df V(sqrtf(1.f - NdotV * NdotV), NdotV, 0.f);
	float DFG = 0.f;
	for (unsigned sample = 0; sample < 1024; sample++)
	{
		std::pair<float, float> ThetaPhi = ImportanceSamplingCos(HammersleySequence(sample, 1024));
		float Theta = ThetaPhi.first, Phi = ThetaPhi.second;
		irr::core::vector3df L(sinf(Theta) * cosf(Phi), cosf(Theta), sinf(Theta) * sinf(Phi));
		float NdotL = L.Y;
		if (NdotL > 0.f)
		{
			irr::core::vector3df H = (L + V).normalize();
			float LdotH = L.dotProduct(H);
			float f90 = .5f + 2.f * LdotH * LdotH * roughness * roughness;
			DFG += (1.f + (f90 - 1.f) * (1.f - powf(NdotL, 5.f))) * (1.f + (f90 - 1.f) * (1.f - powf(NdotV, 5.f)));
		}
	}
	return DFG / 1024;
}

/** Generate the Look Up Table for the DFG texture.
	DFG Texture is used to compute diffuse and specular response from environmental lighting. */
IImage getDFGLUT(size_t DFG_LUT_size)
{
	IImage DFG_LUT_texture;
	DFG_LUT_texture.Format = irr::video::ECF_R32G32B32A32F;
	DFG_LUT_texture.Type = TextureType::TEXTURE2D;
	float *texture_content = new float[4 * DFG_LUT_size * DFG_LUT_size];

	PackedMipMapLevel LUT = {
	  DFG_LUT_size,
	  DFG_LUT_size,
	  texture_content,
	  4 * DFG_LUT_size * DFG_LUT_size * sizeof(float)
	};
	DFG_LUT_texture.Layers.push_back({ LUT });

#pragma omp parallel for
	for (int i = 0; i < int(DFG_LUT_size); i++)
	{
		float roughness = .05f + .95f * float(i) / float(DFG_LUT_size - 1);
		for (unsigned j = 0; j < DFG_LUT_size; j++)
		{
			float NdotV = float(1 + j) / float(DFG_LUT_size + 1);
			std::pair<float, float> DFG = getSpecularDFG(roughness, NdotV);
			texture_content[4 * (i * DFG_LUT_size + j)] = DFG.first;
			texture_content[4 * (i * DFG_LUT_size + j) + 1] = DFG.second;
			texture_content[4 * (i * DFG_LUT_size + j) + 2] = getDiffuseDFG(roughness, NdotV);
			texture_content[4 * (i * DFG_LUT_size + j) + 3] = 0.;
		}
	}

	return DFG_LUT_texture;
}
#endif

