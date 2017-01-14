#include "mesh.h"
#include <Scene\MeshSceneNode.h>

extern "C" {
__declspec(dllexport) void draw_vulkan_mesh(void *);
__declspec(dllexport) void set_horizontal_angle(void *ptr, float);
__declspec(dllexport) void *get_xue(void *ptr);
__declspec(dllexport) void set_rotation_on_node(void *ptr, float x, float y,
                                                float z);
__declspec(dllexport) void destroy_vulkan_mesh(void *ptr);
}

void draw_vulkan_mesh(void *ptr) {
  MeshSample *mesh_ptr = (MeshSample *)ptr;
  mesh_ptr->Draw();
}

void set_horizontal_angle(void *ptr, float ha) {
  MeshSample *mesh_ptr = (MeshSample *)ptr;
  mesh_ptr->horizon_angle = ha;
}

void *get_xue(void *ptr) {
  MeshSample *mesh_ptr = (MeshSample *)ptr;
  return mesh_ptr->xue;
}

void set_rotation_on_node(void *ptr, float x, float y, float z) {
  irr::scene::IMeshSceneNode *node_ptr = (irr::scene::IMeshSceneNode *)ptr;
  node_ptr->setRotation(glm::vec3(x, y, z));
}

void destroy_vulkan_mesh(void *ptr) { delete (MeshSample *)ptr; }
