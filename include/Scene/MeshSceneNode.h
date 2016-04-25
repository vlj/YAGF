// Copyright (C) 2002-2012 Nikolaus Gebhardt
// Copyright (C) 2015 Vincent Lejeune
// Contains code from the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef MESHSCENENODE_H
#define MESHSCENENODE_H

#ifdef D3D12
#include <API/d3dapi.h>
#else
#include <API/vkapi.h>
#endif
#include <API/GfxApi.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <Core/SColor.h>
#include <Maths/matrix4.h>
//#include <Core/ISkinnedMesh.h>
#include <Scene/ISceneNode.h>



namespace irr
{
	namespace scene
	{
		//! A scene node displaying a static mesh
		struct IMeshSceneNode : public ISceneNode
		{
			buffer_t object_data_buffer;
			buffer_t vertex_pos;
			buffer_t vertex_uv0;
			buffer_t vertex_normal;
			buffer_t index_buffer;
			size_t total_index_cnt;
			std::vector<std::tuple<buffer_t, uint64_t, uint32_t, uint32_t> > vertex_buffers_info;

			std::vector<std::tuple<size_t, size_t, size_t> > meshOffset;
			std::vector<uint32_t> texture_mapping;
			std::vector<image_t> Textures;
			std::vector<buffer_t> upload_buffers;
#ifndef D3D12
			VkDescriptorSet object_descriptor_set;
			std::vector<VkDescriptorSet> mesh_descriptor_set;
			std::vector<std::shared_ptr<vulkan_wrapper::image_view> > Textures_views;
#endif
		public:
			//! Constructor
			IMeshSceneNode(ISceneNode* parent,
				device_t dev,
				const aiScene*,
				descriptor_storage_t heap,
				command_list_t initialisation_command,
				const core::vector3df& position = core::vector3df(0, 0, 0),
				const core::vector3df& rotation = core::vector3df(0, 0, 0),
				const core::vector3df& scale = core::vector3df(1.f, 1.f, 1.f));

			~IMeshSceneNode();

			void fill_draw_command_list(command_list_t current_cmd_list, pipeline_layout_t object_sig);
			void update_object_mesh_data(device_t dev);
		};

	} // end namespace scene
} // end namespace irr


#endif
