// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include <Scene/MeshSceneNode.h>
#include <Scene/textures.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <Loaders/DDS.h>
#include <tuple>
#include <array>
#include <unordered_map>

#define SAMPLE_PATH "..\\..\\..\\examples\\assets\\"


struct ObjectData
{
	float ModelMatrix[16];
	float InverseModelMatrix[16];
};

namespace irr
{
	namespace scene
	{
		//! Constructor
		/** Use setMesh() to set the mesh to display.
		*/
		IMeshSceneNode::IMeshSceneNode(device_t& dev, const aiScene*model, command_list_t& upload_cmd_list, descriptor_storage_t& heap,
			descriptor_set_layout* object_set, descriptor_set_layout* model_set,
			ISceneNode* parent,
			const glm::vec3& position,
			const glm::vec3& rotation,
			const glm::vec3& scale)
			: ISceneNode(parent, position, rotation, scale)
		{
			object_matrix = dev.create_buffer(sizeof(ObjectData), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, usage_uniform);
			object_descriptor_set = heap.allocate_descriptor_set_from_cbv_srv_uav_heap(3, { object_set }, 2);
			dev.set_constant_buffer_view(*object_descriptor_set, 0, 0, *object_matrix, sizeof(ObjectData));
			dev.set_constant_buffer_view(*object_descriptor_set, 1, 1, *object_matrix, sizeof(ObjectData));

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
			for (unsigned int i = 0; i < model->mNumMeshes; ++i)
			{
				const aiMesh *mesh = model->mMeshes[i];
				total_vertex_cnt += mesh->mNumVertices;
				total_index_cnt += mesh->mNumFaces * 3;
			}

			index_buffer = dev.create_buffer(total_index_cnt * sizeof(uint16_t), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, usage_index);
			uint16_t *indexmap = (uint16_t *)index_buffer->map_buffer();
			vertex_pos = dev.create_buffer(total_vertex_cnt * sizeof(aiVector3D), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, usage_vertex);
			aiVector3D *vertex_pos_map = (aiVector3D*)vertex_pos->map_buffer();
			vertex_normal = dev.create_buffer(total_vertex_cnt * sizeof(aiVector3D), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, usage_vertex);
			aiVector3D *vertex_normal_map = (aiVector3D*)vertex_normal->map_buffer();
			vertex_uv0 = dev.create_buffer(total_vertex_cnt * sizeof(aiVector3D), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, usage_vertex);
			aiVector3D *vertex_uv_map = (aiVector3D*)vertex_uv0->map_buffer();

			uint32_t basevertex = 0;
			uint32_t baseindex = 0;

			for (unsigned int i = 0; i < model->mNumMeshes; ++i)
			{
				const aiMesh *mesh = model->mMeshes[i];
				for (unsigned int vtx = 0; vtx < mesh->mNumVertices; ++vtx)
				{
					vertex_pos_map[basevertex + vtx] = mesh->mVertices[vtx];
					vertex_normal_map[basevertex + vtx] = mesh->mNormals[vtx];
					vertex_uv_map[basevertex + vtx] = mesh->mTextureCoords[0][vtx];
				}
				for (unsigned int idx = 0; idx < mesh->mNumFaces; ++idx)
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

			index_buffer->unmap_buffer();
			vertex_pos->unmap_buffer();
			vertex_normal->unmap_buffer();
			vertex_uv0->unmap_buffer();
			// TODO: Upload to GPUmem

			vertex_buffers_info.emplace_back(*vertex_pos, 0, static_cast<uint32_t>(sizeof(aiVector3D)), static_cast<uint32_t>(total_vertex_cnt * sizeof(aiVector3D)));
			vertex_buffers_info.emplace_back(*vertex_normal, 0, static_cast<uint32_t>(sizeof(aiVector3D)), static_cast<uint32_t>(total_vertex_cnt * sizeof(aiVector3D)));
			vertex_buffers_info.emplace_back(*vertex_uv0, 0, static_cast<uint32_t>(sizeof(aiVector3D)), static_cast<uint32_t>(total_vertex_cnt * sizeof(aiVector3D)));

			// Texture
			for (unsigned int texture_id = 0; texture_id < model->mNumMaterials; ++texture_id)
			{
				aiString path;
				model->mMaterials[texture_id]->GetTexture(aiTextureType_DIFFUSE, 0, &path);
				std::string texture_path(path.C_Str());

				std::unique_ptr<image_t> texture;
				std::unique_ptr<buffer_t> upload_buffer;
				std::tie(texture, upload_buffer) = load_texture(dev, SAMPLE_PATH + texture_path.substr(0, texture_path.find_last_of('.')) + ".DDS", upload_cmd_list);
				auto&& mesh_descriptor = heap.allocate_descriptor_set_from_cbv_srv_uav_heap(13 + texture_id, { model_set }, 1);
				mesh_descriptor_set.push_back(std::move(mesh_descriptor));

				Textures_views.push_back(dev.create_image_view(*texture, irr::video::ECF_BC1_UNORM_SRGB, 0, 9, 0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D));
				dev.set_image_view(*mesh_descriptor_set.back(), 0, 2, *Textures_views.back());
				upload_buffers.push_back(std::move(upload_buffer));
				Textures.push_back(std::move(texture));
			}
		}

		IMeshSceneNode::~IMeshSceneNode()
		{

		}

		void IMeshSceneNode::fill_draw_command(command_list_t& current_cmd_list, pipeline_layout_t& object_sig)
		{
			current_cmd_list.bind_graphic_descriptor(1, *object_descriptor_set, object_sig);
			current_cmd_list.bind_index_buffer(*index_buffer, 0, total_index_cnt * sizeof(uint16_t), irr::video::E_INDEX_TYPE::EIT_16BIT);
			current_cmd_list.bind_vertex_buffers(0, vertex_buffers_info);

			for (unsigned i = 0; i < meshOffset.size(); i++)
			{
				current_cmd_list.bind_graphic_descriptor(0, *mesh_descriptor_set[texture_mapping[i]], object_sig);
				current_cmd_list.draw_indexed(std::get<0>(meshOffset[i]), 1, std::get<2>(meshOffset[i]), std::get<1>(meshOffset[i]), 0);
			}
		}

		void IMeshSceneNode::update_constant_buffers(device_t& dev)
		{
			ObjectData *cbufdata = static_cast<ObjectData*>(object_matrix->map_buffer());
			updateAbsolutePosition();
			glm::mat4 Model = getAbsoluteTransformation();
			glm::mat4 InvModel = glm::inverse(Model);

			memcpy(cbufdata->ModelMatrix, &Model, 16 * sizeof(float));
			memcpy(cbufdata->InverseModelMatrix, &InvModel, 16 * sizeof(float));
			object_matrix->unmap_buffer();
		}

	}
}