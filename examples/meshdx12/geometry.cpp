// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include "geometry.h"

object::object(device_t dev, const aiScene* model)
{

	// Format Weight

	/*        std::vector<std::vector<irr::video::SkinnedVertexData> > weightsList;
	for (auto weightbuffer : loader->AnimatedMesh.WeightBuffers)
	{
	std::vector<irr::video::SkinnedVertexData> weights;
	for (unsigned j = 0; j < weightbuffer.size(); j += 4)
	{
	irr::video::SkinnedVertexData tmp = {
	weightbuffer[j].Index, weightbuffer[j].Weight,
	weightbuffer[j + 1].Index, weightbuffer[j + 1].Weight,
	weightbuffer[j + 2].Index, weightbuffer[j + 2].Weight,
	weightbuffer[j + 3].Index, weightbuffer[j + 3].Weight,
	};
	weights.push_back(tmp);
	}
	weightsList.push_back(weights);
	}*/

	size_t total_vertex_cnt = 0;
	total_index_cnt = 0;
	for (int i = 0; i < model->mNumMeshes; ++i)
	{
		const aiMesh *mesh = model->mMeshes[i];
		total_vertex_cnt += mesh->mNumVertices;
		total_index_cnt += mesh->mNumFaces * 3;
	}

	index_buffer = create_buffer(dev, total_index_cnt * sizeof(uint16_t));
	uint16_t *indexmap = (uint16_t *)map_buffer(dev, index_buffer);
	vertex_pos = create_buffer(dev, total_vertex_cnt * sizeof(aiVector3D));
	aiVector3D *vertex_pos_map = (aiVector3D*)map_buffer(dev, vertex_pos);
	vertex_normal = create_buffer(dev, total_vertex_cnt * sizeof(aiVector3D));
	aiVector3D *vertex_normal_map = (aiVector3D*)map_buffer(dev, vertex_normal);
	vertex_uv0 = create_buffer(dev, total_vertex_cnt * sizeof(aiVector3D));
	aiVector3D *vertex_uv_map = (aiVector3D*)map_buffer(dev, vertex_uv0);

	size_t basevertex = 0, baseindex = 0;

	for (int i = 0; i < model->mNumMeshes; ++i)
	{
		const aiMesh *mesh = model->mMeshes[i];
		for (int vtx = 0; vtx < mesh->mNumVertices; ++vtx)
		{
			vertex_pos_map[basevertex + vtx] = mesh->mVertices[vtx];
			vertex_normal_map[basevertex + vtx] = mesh->mNormals[vtx];
			vertex_uv_map[basevertex + vtx] = mesh->mTextureCoords[0][vtx];
		}
		for (int idx = 0; idx < mesh->mNumFaces; ++idx)
		{
			indexmap[baseindex + 3 * idx] = mesh->mFaces[idx].mIndices[0];
			indexmap[baseindex + 3 * idx + 1] = mesh->mFaces[idx].mIndices[1];
			indexmap[baseindex + 3 * idx + 2] = mesh->mFaces[idx].mIndices[2];
		}
		meshOffset.push_back(std::make_tuple(mesh->mNumFaces * 3, basevertex, baseindex));
		basevertex += mesh->mNumVertices;
		baseindex += mesh->mNumFaces * 3;
		texture_mapping.push_back(mesh->mMaterialIndex);
	}

	unmap_buffer(dev, index_buffer);
	unmap_buffer(dev, vertex_pos);
	unmap_buffer(dev, vertex_normal);
	unmap_buffer(dev, vertex_uv0);
	// TODO: Upload to GPUmem

	vertex_buffers_info.emplace_back(vertex_pos, 0, static_cast<uint32_t>(sizeof(aiVector3D)), static_cast<uint32_t>(total_vertex_cnt * sizeof(aiVector3D)));
	vertex_buffers_info.emplace_back(vertex_normal, 0, static_cast<uint32_t>(sizeof(aiVector3D)), static_cast<uint32_t>(total_vertex_cnt * sizeof(aiVector3D)));
	vertex_buffers_info.emplace_back(vertex_uv0, 0, static_cast<uint32_t>(sizeof(aiVector3D)), static_cast<uint32_t>(total_vertex_cnt * sizeof(aiVector3D)));
}
