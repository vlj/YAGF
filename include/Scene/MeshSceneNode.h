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

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <tuple>
#include <array>
#include <unordered_map>
#include <Core/SColor.h>
#include <Maths/matrix4.h>
//#include <Core/ISkinnedMesh.h>
#include <Scene/ISceneNode.h>

namespace irr
{
	namespace scene
	{
		//! A scene node displaying a static mesh
		class IMeshSceneNode : public ISceneNode
		{
			std::vector<std::tuple<uint32_t, uint32_t, uint32_t> > meshOffset;
			std::vector<std::unique_ptr<buffer_t>> upload_buffers;

			std::unique_ptr<buffer_t> vertex_pos;
			std::unique_ptr<buffer_t> vertex_uv0;
			std::unique_ptr<buffer_t> vertex_normal;
			std::unique_ptr<buffer_t> index_buffer;
			uint32_t total_index_cnt;
			std::vector<std::tuple<buffer_t*, uint64_t, uint32_t, uint32_t> > vertex_buffers_info;

			std::vector<uint32_t> texture_mapping;
			std::vector<image_t> Textures;

			std::unique_ptr<buffer_t> object_matrix;
#ifndef D3D12
			std::vector<VkDescriptorSet> texture_descriptor_set;
			std::vector<std::shared_ptr<vulkan_wrapper::image_view> > Textures_views;
			std::vector<VkDescriptorSet> mesh_descriptor_set;
			VkDescriptorSet object_descriptor_set;
#endif
		public:

			//! Constructor
			/** Use setMesh() to set the mesh to display.
			*/
			IMeshSceneNode(device_t dev, const aiScene*, command_list_t upload_cmd_list, descriptor_storage_t heap,
#ifndef D3D12
				vulkan_wrapper::pipeline_descriptor_set* object_set, vulkan_wrapper::pipeline_descriptor_set* model_set,
#endif
				ISceneNode* parent,
				const core::vector3df& position = core::vector3df(0, 0, 0),
				const core::vector3df& rotation = core::vector3df(0, 0, 0),
				const core::vector3df& scale = core::vector3df(1.f, 1.f, 1.f));

			~IMeshSceneNode();
			void render() {}

			void fill_draw_command(device_t dev, command_list_t cmd_list, pipeline_layout_t object_sig, descriptor_storage_t heap);
			void update_constant_buffers(device_t dev);
		};

	} // end namespace scene
} // end namespace irr


#endif
