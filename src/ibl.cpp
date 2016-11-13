// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#include <Scene/IBL.h>
#include <cmath>
#include <glm/mat4x4.hpp>
#include <set>
#include <d3dcompiler.h>

const auto dfg_code = std::vector<uint32_t>
#include <generatedShaders\dfg.h>
;

const auto importance_sampling_specular_code = std::vector<uint32_t>
#include <generatedShaders\importance_sampling_specular.h>
;

const auto computesh_code = std::vector<uint32_t>
#include <generatedShaders\computesh.h>
;

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

	auto get_compute_sh_pipeline_state(device_t& dev, pipeline_layout_t& pipeline_layout)
	{
		auto pso_desc = compute_pipeline_state_description{}
			.set_compute_shader(computesh_code);
		return dev.create_compute_pso(pso_desc);
	}

	auto ImportanceSamplingForSpecularCubemap(device_t& dev, pipeline_layout_t& pipeline_layout)
	{
		auto pso_desc = compute_pipeline_state_description{}
			.set_compute_shader(importance_sampling_specular_code);
		return dev.create_compute_pso(pso_desc);
	}

	auto dfg_building_pso(device_t& dev, pipeline_layout_t& pipeline_layout)
	{
		auto pso_desc = compute_pipeline_state_description{}
			.set_compute_shader(dfg_code);
		return dev.create_compute_pso(pso_desc);
	}

	auto getPermutationMatrix(size_t indexX, float valX, size_t indexY, float valY, size_t indexZ, float valZ)
	{
		glm::mat4 resultMat;
		// TODO: Don't use this reinterpret cast
		float *M = reinterpret_cast<float*>(&resultMat);
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
	object_set = dev.get_object_descriptor_set(object_descriptor_set_type);
	sampler_set = dev.get_object_descriptor_set(sampler_descriptor_set_type);
	compute_sh_sig = dev.create_pipeline_layout(std::vector<const descriptor_set_layout*>{ object_set.get(), sampler_set.get()});
	compute_sh_pso = get_compute_sh_pipeline_state(dev, *compute_sh_sig);
	face_set = dev.get_object_descriptor_set(face_set_type);
	mipmap_set = dev.get_object_descriptor_set(mipmap_set_type);
	uav_set = dev.get_object_descriptor_set(uav_set_type);
	sampler_set = dev.get_object_descriptor_set(sampler_descriptor_set_type);
	importance_sampling_sig = dev.create_pipeline_layout(std::vector<const descriptor_set_layout*>{ face_set.get(), mipmap_set.get(), uav_set.get(), sampler_set.get() });
	importance_sampling = ImportanceSamplingForSpecularCubemap(dev, *importance_sampling_sig);

	dfg_set = dev.get_object_descriptor_set(dfg_input);
	dfg_building_sig = dev.create_pipeline_layout(std::vector<const descriptor_set_layout*>{ dfg_set.get() });
	pso = dfg_building_pso(dev, *dfg_building_sig);

	srv_cbv_uav_heap = dev.create_descriptor_storage(100, { { RESOURCE_VIEW::CONSTANTS_BUFFER, 20 },{ RESOURCE_VIEW::SHADER_RESOURCE, 20 },{ RESOURCE_VIEW::UAV_BUFFER, 1 },{ RESOURCE_VIEW::TEXEL_BUFFER, 10 }, { RESOURCE_VIEW::UAV_IMAGE, 50 } });
	sampler_heap = dev.create_descriptor_storage(10, { { RESOURCE_VIEW::SAMPLER, 1 } });

	sampler_descriptors = sampler_heap->allocate_descriptor_set_from_cbv_srv_uav_heap(0, { sampler_set.get() }, 1);

	anisotropic_sampler = dev.create_sampler(SAMPLER_TYPE::ANISOTROPIC);
	dev.set_sampler(*sampler_descriptors, 0, 4, *anisotropic_sampler);

	compute_sh_cbuf = dev.create_buffer(sizeof(int), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, none);
	dfg_cbuf = dev.create_buffer(sizeof(float), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, none);

	auto M = std::array<glm::mat4, 6>{
		getPermutationMatrix(2, -1., 1, -1., 0, 1.),
		getPermutationMatrix(2, 1., 1, -1., 0, -1.),
		getPermutationMatrix(0, 1., 2, 1., 1, 1.),
		getPermutationMatrix(0, 1., 2, -1., 1, -1.),
		getPermutationMatrix(0, 1., 1, -1., 2, 1.),
		getPermutationMatrix(0, -1., 1, -1., 2, -1.),
	};

	for (unsigned i = 0; i < 6; i++)
	{
		permutation_matrix[i] = dev.create_buffer(sizeof(PermutationMatrix), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, none);
		memcpy(permutation_matrix[i]->map_buffer(), &M[i], 16 * sizeof(float));
		permutation_matrix[i]->unmap_buffer();
	}

	hammersley_sequence_buffer = dev.create_buffer(2048 * sizeof(float), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, usage_texel_buffer);
	float *tmp = reinterpret_cast<float*>(hammersley_sequence_buffer->map_buffer());
	for (unsigned j = 0; j < 1024; j++)
	{
		std::pair<float, float> sample = HammersleySequence(j, 1024);
		tmp[2 * j] = sample.first;
		tmp[2 * j + 1] = sample.second;
	}
	hammersley_sequence_buffer->unmap_buffer();
	hammersley_sequence_buffer_view = dev.create_buffer_view(*hammersley_sequence_buffer, irr::video::ECF_R32G32F, 0, 2048 * sizeof(float));

	for (unsigned i = 0; i < 8; i++)
	{
		per_level_cbuffer[i] = dev.create_buffer(sizeof(per_level_importance_sampling_data), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, none);
		per_level_importance_sampling_data* tmp = (per_level_importance_sampling_data*)per_level_cbuffer[i]->map_buffer();
		float viewportSize = float(1 << (8 - i));
		tmp->size = viewportSize;
		tmp->alpha = .05f + .95f * i / 8.f;
		per_level_cbuffer[i]->unmap_buffer();
	}
}

struct SHCoefficients
{
	float Red[9];
	float Green[9];
	float Blue[9];
};

std::unique_ptr<allocated_descriptor_set> ibl_utility::get_compute_sh_descriptor(device_t &dev, buffer_t &constant_buffer, image_view_t& probe_view, buffer_t& sh_buffer)
{
	auto input_descriptors = srv_cbv_uav_heap->allocate_descriptor_set_from_cbv_srv_uav_heap(0, { object_set.get() }, 3);
	dev.set_constant_buffer_view(*input_descriptors, 0, 0, constant_buffer, sizeof(int));
	dev.set_image_view(*input_descriptors, 1, 1, probe_view);
	dev.set_uav_buffer_view(*input_descriptors, 2, 2, sh_buffer, 0, sizeof(SH));
	return input_descriptors;
}

std::unique_ptr<allocated_descriptor_set> ibl_utility::get_dfg_input_descriptor_set(device_t & dev, buffer_t& constant_buffer, image_view_t &DFG_LUT_view)
{
	auto dfg_input_descriptor_set = srv_cbv_uav_heap->allocate_descriptor_set_from_cbv_srv_uav_heap(0, { dfg_set.get() }, 3);
	dev.set_constant_buffer_view(*dfg_input_descriptor_set, 0, 0, constant_buffer, sizeof(float));
	dev.set_uniform_texel_buffer_view(*dfg_input_descriptor_set, 1, 1, *hammersley_sequence_buffer_view);
	dev.set_uav_image_view(*dfg_input_descriptor_set, 2, 2, DFG_LUT_view);

	return dfg_input_descriptor_set;
}

std::unique_ptr<buffer_t> ibl_utility::computeSphericalHarmonics(device_t& dev, command_list_t& cmd_list, image_view_t& probe_view, size_t edge_size)
{
	void* tmp = compute_sh_cbuf->map_buffer();
	float cube_size = static_cast<float>(edge_size) / 10.f;
	memcpy(tmp, &cube_size, sizeof(int));
	compute_sh_cbuf->unmap_buffer();

	auto sh_buffer = dev.create_buffer(sizeof(SH), irr::video::E_MEMORY_POOL::EMP_GPU_LOCAL, usage_uav);

	cmd_list.set_compute_pipeline_layout(*compute_sh_sig);
	cmd_list.set_descriptor_storage_referenced(*srv_cbv_uav_heap, sampler_heap.get());
#ifdef D3D12
	cmd_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(sh_buffer->object, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
#endif
	cmd_list.set_compute_pipeline(*compute_sh_pso);
	cmd_list.bind_compute_descriptor(0, *get_compute_sh_descriptor(dev, *compute_sh_cbuf, probe_view, *sh_buffer), *compute_sh_sig);
	cmd_list.bind_compute_descriptor(1, *sampler_descriptors, *compute_sh_sig);

	cmd_list.dispatch(1, 1, 1);
	// for debug
	//	std::unique_ptr<buffer_t> sh_buffer_readback = create_buffer(dev, sizeof(SH), irr::video::E_MEMORY_POOL::EMP_CPU_READABLE, usage_transfer_dst);
	//	copy_buffer(command_list.get(), sh_buffer.get(), 0, sh_buffer_readback.get(), 0, sizeof(SH));
/*	SHCoefficients Result;
	float* Shval = (float*)map_buffer(dev, sh_buffer_readback.get());
	memcpy(Result.Blue, Shval, 9 * sizeof(float));
	memcpy(Result.Green, &Shval[9], 9 * sizeof(float));
	memcpy(Result.Red, &Shval[18], 9 * sizeof(float));*/
	//	command_list->object->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(sh_buffer->object, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
	//	command_list->object->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(sh_buffer->object));

	return std::move(sh_buffer);
}

std::unique_ptr<image_t> ibl_utility::generateSpecularCubemap(device_t& dev, command_list_t& cmd_list, image_view_t& probe_view)
{
	size_t cubemap_size = 256;
	auto permutation_matrix_descriptors = std::array<std::unique_ptr<allocated_descriptor_set>, 6>{};
	for (unsigned i = 0; i < 6; i++)
	{
		permutation_matrix_descriptors[i] = srv_cbv_uav_heap->allocate_descriptor_set_from_cbv_srv_uav_heap(2 * i, { face_set.get() }, 2);
		dev.set_image_view(*permutation_matrix_descriptors[i], 0, 0, probe_view);
		dev.set_constant_buffer_view(*permutation_matrix_descriptors[i], 1, 1, *permutation_matrix[i], sizeof(PermutationMatrix));
	}

	auto result = dev.create_image(irr::video::ECF_R16G16B16A16F, 256, 256, 8, 6, usage_cube | usage_sampled | usage_uav, nullptr);
	cmd_list.set_compute_pipeline(*importance_sampling);
	cmd_list.set_compute_pipeline_layout(*importance_sampling_sig);
	cmd_list.set_descriptor_storage_referenced(*srv_cbv_uav_heap, sampler_heap.get());

//	bind_compute_descriptor(command_list.get(), 0, image_descriptors, importance_sampling_sig);
	cmd_list.bind_compute_descriptor(3, *sampler_descriptors, *importance_sampling_sig);

	for (unsigned level = 0; level < 8; level++)
	{
		auto per_level_descriptor = srv_cbv_uav_heap->allocate_descriptor_set_from_cbv_srv_uav_heap(2 * level + 12, { mipmap_set.get() }, 2);
		dev.set_constant_buffer_view(*per_level_descriptor, 1, 5, *per_level_cbuffer[level], sizeof(float));
		dev.set_uniform_texel_buffer_view(*per_level_descriptor, 0, 2, *hammersley_sequence_buffer_view);
		cmd_list.bind_compute_descriptor(1, *per_level_descriptor, *importance_sampling_sig);
		for (unsigned face = 0; face < 6; face++)
		{
			cmd_list.set_pipeline_barrier(*result, RESOURCE_USAGE::undefined, RESOURCE_USAGE::uav, face + level * 6, irr::video::E_ASPECT::EA_COLOR);

			auto level_face_descriptor = srv_cbv_uav_heap->allocate_descriptor_set_from_cbv_srv_uav_heap(face + level * 6 + 28, { uav_set.get() }, 1);
			uav_views[face + level * 6] = dev.create_image_view(*result, irr::video::ECF_R16G16B16A16F, level, 1, face, 1, irr::video::E_TEXTURE_TYPE::ETT_CUBE);
			dev.set_uav_image_view(*level_face_descriptor, 0, 3, *uav_views[face + level * 6]);

			cmd_list.bind_compute_descriptor(0, *permutation_matrix_descriptors[face], *importance_sampling_sig);
			cmd_list.bind_compute_descriptor(2, *level_face_descriptor, *importance_sampling_sig);
			cmd_list.dispatch(256 >> level, 256 >> level, 1);

			cmd_list.set_pipeline_barrier(*result, RESOURCE_USAGE::uav, RESOURCE_USAGE::READ_GENERIC, face + level * 6, irr::video::E_ASPECT::EA_COLOR);
		}
	}
	return result;
}

/** Generate the Look Up Table for the DFG texture.
	DFG Texture is used to compute diffuse and specular response from environmental lighting. */
std::tuple<std::unique_ptr<image_t>, std::unique_ptr<image_view_t>> ibl_utility::getDFGLUT(device_t& dev, command_list_t& cmd_list, uint32_t DFG_LUT_size)
{
	auto DFG_LUT_texture = dev.create_image(irr::video::ECF_R32G32B32A32F, DFG_LUT_size, DFG_LUT_size, 1, 1, usage_sampled | usage_uav, nullptr);
	auto texture_view = dev.create_image_view(*DFG_LUT_texture, irr::video::ECF_R32G32B32A32F, 0, 1, 0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D);

	void* tmp = dfg_cbuf->map_buffer();
	float sz = static_cast<float>(DFG_LUT_size);
	memcpy(tmp, &sz, sizeof(float));
	dfg_cbuf->unmap_buffer();

	cmd_list.set_pipeline_barrier(*DFG_LUT_texture, RESOURCE_USAGE::undefined, RESOURCE_USAGE::uav, 0, irr::video::E_ASPECT::EA_COLOR);

	auto dfg_input_descriptor_set = get_dfg_input_descriptor_set(dev, *dfg_cbuf, *texture_view);
	cmd_list.set_compute_pipeline(*pso);
	cmd_list.set_compute_pipeline_layout(*dfg_building_sig);
	cmd_list.set_descriptor_storage_referenced(*srv_cbv_uav_heap);
	cmd_list.bind_compute_descriptor(0, *dfg_input_descriptor_set, *dfg_building_sig);

	cmd_list.dispatch(DFG_LUT_size, DFG_LUT_size, 1);
	cmd_list.set_pipeline_barrier(*DFG_LUT_texture, RESOURCE_USAGE::uav, RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_COLOR);

	return std::make_tuple(std::move(DFG_LUT_texture), std::move(texture_view));
}
