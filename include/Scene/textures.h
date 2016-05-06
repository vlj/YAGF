// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <Loaders/DDS.h>
#include <tuple>
#include <array>
#include <unordered_map>

#ifdef D3D12
#include <API/d3dapi.h>
#else
#include <API/vkapi.h>
#endif

std::tuple<std::unique_ptr<image_t>, std::unique_ptr<buffer_t>> load_texture(device_t& dev, const std::string &texture_name, command_list_t& upload_command_list);
