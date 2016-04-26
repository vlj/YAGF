// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include <Scene/textures.h>


std::tuple<image_t, buffer_t> load_texture(device_t dev, const std::string &texture_name, command_list_t upload_command_list)
{
	std::ifstream DDSFile(texture_name, std::ifstream::binary);
	irr::video::CImageLoaderDDS DDSPic(DDSFile);

	uint32_t width = static_cast<uint32_t>(DDSPic.getLoadedImage().Layers[0][0].Width);
	uint32_t height = static_cast<uint32_t>(DDSPic.getLoadedImage().Layers[0][0].Height);
	uint16_t mipmap_count = static_cast<uint16_t>(DDSPic.getLoadedImage().Layers[0].size());

	bool is_cubemap = DDSPic.getLoadedImage().Type == TextureType::CUBEMAP;
	uint16_t layer_count = is_cubemap ? 6 : 1;

	buffer_t upload_buffer = create_buffer(dev, width * height * 3 * 6);

	void *pointer = map_buffer(dev, upload_buffer);

	size_t offset_in_texram = 0;

	uint32_t block_height = 4;
	uint32_t block_width = 4;
	uint32_t block_size = 8;
	std::vector<MipLevelData> Mips;
	for (unsigned face = 0; face < layer_count; face++)
	{
		for (unsigned i = 0; i < mipmap_count; i++)
		{
			const IImage &image = DDSPic.getLoadedImage();
			struct PackedMipMapLevel miplevel = image.Layers[face][i];
			// Offset needs to be aligned to 512 bytes
			offset_in_texram = (offset_in_texram + 511) & ~511;
			// Row pitch is always a multiple of 256
			uint32_t height_in_blocks = static_cast<uint32_t>(image.Layers[face][i].Height + block_height - 1) / block_height;
			uint32_t width_in_blocks = static_cast<uint32_t>(image.Layers[face][i].Width + block_width - 1) / block_width;
			uint32_t height_in_texram = height_in_blocks * block_height;
			uint32_t width_in_texram = width_in_blocks * block_width;
			uint32_t rowPitch = width_in_blocks * block_size;
			rowPitch = (rowPitch + 255) & ~255;
			MipLevelData mml = { offset_in_texram, width_in_texram, height_in_texram, rowPitch };
			Mips.push_back(mml);
			for (unsigned row = 0; row < height_in_blocks; row++)
			{
				memcpy(((char*)pointer) + offset_in_texram, ((char*)miplevel.Data) + row * width_in_blocks * block_size, width_in_blocks * block_size);
				offset_in_texram += rowPitch;
			}
		}
	}
	unmap_buffer(dev, upload_buffer);

	image_t texture = create_image(dev, irr::video::ECF_BC1_UNORM_SRGB,
		width, height, mipmap_count, layer_count, usage_sampled | usage_transfer_dst | is_cubemap ? usage_cube : 0,
		nullptr);

	uint32_t miplevel = 0;
	for (const MipLevelData mipmapData : Mips)
	{
		set_pipeline_barrier(dev, upload_command_list, texture, RESOURCE_USAGE::undefined, RESOURCE_USAGE::COPY_DEST, miplevel, irr::video::E_ASPECT::EA_COLOR);
		copy_buffer_to_image_subresource(upload_command_list, texture, miplevel, upload_buffer, mipmapData.Offset, mipmapData.Width, mipmapData.Height, mipmapData.RowPitch, irr::video::ECF_BC1_UNORM_SRGB);
		set_pipeline_barrier(dev, upload_command_list, texture, RESOURCE_USAGE::COPY_DEST, RESOURCE_USAGE::READ_GENERIC, miplevel, irr::video::E_ASPECT::EA_COLOR);
		miplevel++;
	}
	return std::make_tuple(texture, upload_buffer);
}