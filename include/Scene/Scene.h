// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __SCENE_H__
#define __SCENE_H__

#include <Scene\ISceneNode.h>
#include <Scene\MeshSceneNode.h>
#include <memory>

namespace irr
{
	namespace scene
	{
		class Scene
		{
		private:
			std::list<std::unique_ptr<irr::scene::IMeshSceneNode> > Nodes;
		public:
			Scene();
			~Scene();

			void update(device_t &dev);
			void fill_gbuffer_filling_command(command_list_t& cmd_list, pipeline_layout_t& object_sig);

			irr::scene::IMeshSceneNode *addMeshSceneNode(
				std::unique_ptr<irr::scene::IMeshSceneNode> &&mesh,
				irr::scene::ISceneNode* parent,
				const glm::vec3& position = glm::vec3(0, 0, 0),
				const glm::vec3& rotation = glm::vec3(0, 0, 0),
				const glm::vec3& scale = glm::vec3(1.f, 1.f, 1.f));
		};

	}
}
#endif