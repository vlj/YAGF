// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <tuple>
#include <array>
#include <unordered_map>
#include <API/GfxApi.h>

std::tuple<std::unique_ptr<image_t>, std::unique_ptr<buffer_t>> load_texture(device_t& dev, std::string &&texture_name, command_list_t& upload_command_list);
