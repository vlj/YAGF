// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include <Scene\Scene.h>
#include <algorithm>
#include "..\include\Scene\Scene.h"

struct ViewBuffer
{
	float ViewProj[16];
};

using namespace irr::scene;

Scene::Scene()
{

}

Scene::~Scene()
{
}

void Scene::update(device_t &dev)
{
	std::for_each(Nodes.begin(), Nodes.end(), [&dev](std::unique_ptr<IMeshSceneNode> &node) { node->update_constant_buffers(dev); });
}

void irr::scene::Scene::fill_gbuffer_filling_command(command_list_t& cmd_list, pipeline_layout_t object_sig)
{
	std::for_each(Nodes.begin(), Nodes.end(), [&cmd_list, &object_sig](std::unique_ptr<IMeshSceneNode> &node) { node->fill_draw_command(cmd_list, object_sig); });
}

irr::scene::IMeshSceneNode *Scene::addMeshSceneNode(
	std::unique_ptr<irr::scene::IMeshSceneNode> &&mesh,
	irr::scene::ISceneNode* parent,
	const irr::core::vector3df& position,
	const irr::core::vector3df& rotation,
	const irr::core::vector3df& scale)
{
	Nodes.push_back(std::move(mesh));
	return Nodes.back().get();
}
