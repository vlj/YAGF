// Copyright (C) 2013 Patryk Nadrowski
// Heavily based on the DDS loader implemented by Thomas Alten
// and DDS loader from IrrSpintz implemented by Thomas Ince
// Copyright (C) 2015 Vincent Lejeune
// Contains code from the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in License.txt

// Based on Code from Copyright(c) 2003 Randy Reddig
// Based on code from Nvidia's DDS example:
// http://www.nvidia.com/object/dxtc_decompression_code.html

#ifndef __C_IMAGE_LOADER_DDS_H_INCLUDED__
#define __C_IMAGE_LOADER_DDS_H_INCLUDED__

#include <Core/IImage.h>
#include <fstream>
#include <cassert>

namespace irr
{
  namespace video
  {

    // Header flag values
#define DDSD_CAPS			0x00000001
#define DDSD_HEIGHT			0x00000002
#define DDSD_WIDTH			0x00000004
#define DDSD_PITCH			0x00000008
#define DDSD_PIXELFORMAT	0x00001000
#define DDSD_MIPMAPCOUNT	0x00020000
#define DDSD_LINEARSIZE		0x00080000
#define DDSD_DEPTH			0x00800000

        // Pixel format flag values
#define DDPF_ALPHAPIXELS	0x00000001
#define DDPF_ALPHA			0x00000002
#define DDPF_FOURCC			0x00000004
#define DDPF_RGB			0x00000040
#define DDPF_COMPRESSED		0x00000080
#define DDPF_LUMINANCE		0x00020000

        // Caps1 values
#define DDSCAPS1_COMPLEX	0x00000008
#define DDSCAPS1_TEXTURE	0x00001000
#define DDSCAPS1_MIPMAP		0x00400000

        // Caps2 values
#define DDSCAPS2_CUBEMAP            0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX  0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX  0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY  0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY  0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ  0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ  0x00008000
#define DDSCAPS2_VOLUME             0x00200000

        // byte-align structures
#define PACK_STRUCT

        /* structures */

    struct ddsPixelFormat
    {
      unsigned Size;
      unsigned Flags;
      unsigned FourCC;
      unsigned RGBBitCount;
      unsigned RBitMask;
      unsigned GBitMask;
      unsigned BBitMask;
      unsigned ABitMask;
    } PACK_STRUCT;


    struct ddsCaps
    {
      unsigned caps1;
      unsigned caps2;
      unsigned caps3;
      unsigned caps4;
    } PACK_STRUCT;


    struct ddsHeader
    {
      unsigned Size;
      unsigned Flags;
      unsigned Height;
      unsigned Width;
      unsigned PitchOrLinearSize;
      unsigned Depth;
      unsigned MipMapCount;
      unsigned Reserved1[11];
      ddsPixelFormat PixelFormat;
      ddsCaps Caps;
      unsigned Reserved2;
    } PACK_STRUCT;

    struct ddsHeaderDXT10
    {
      unsigned format;
      unsigned resourceDim;
      unsigned miscFlag;
      unsigned arraySize;
      unsigned miscFlag2;
    } PACK_STRUCT;

#ifdef _IRR_COMPILE_WITH_DDS_DECODER_LOADER_

    struct ddsColorBlock
    {
      u16		colors[2];
      u8		row[4];
    } PACK_STRUCT;


    struct ddsAlphaBlockExplicit
    {
      u16		row[4];
    } PACK_STRUCT;


    struct ddsAlphaBlock3BitLinear
    {
      u8		alpha0;
      u8		alpha1;
      u8		stuff[6];
    } PACK_STRUCT;


    struct ddsColor
    {
      u8		r, g, b, a;
    } PACK_STRUCT;

#endif


    // Default alignment
//#include "irrunpack.h"


        /* endian tomfoolery */
    typedef union
    {
      float f;
      char c[4];
    }
    floatSwapUnion;


#ifndef __BIG_ENDIAN__
#ifdef _SGI_SOURCE
#define	__BIG_ENDIAN__
#endif
#endif


#ifdef __BIG_ENDIAN__

    s32   DDSBigLong(s32 src) { return src; }
    s16 DDSBigShort(s16 src) { return src; }
    f32 DDSBigFloat(f32 src) { return src; }

    s32 DDSLittleLong(s32 src)
    {
      return ((src & 0xFF000000) >> 24) |
        ((src & 0x00FF0000) >> 8) |
        ((src & 0x0000FF00) << 8) |
        ((src & 0x000000FF) << 24);
    }

    s16 DDSLittleShort(s16 src)
    {
      return ((src & 0xFF00) >> 8) |
        ((src & 0x00FF) << 8);
    }

    f32 DDSLittleFloat(f32 src)
    {
      floatSwapUnion in, out;
      in.f = src;
      out.c[0] = in.c[3];
      out.c[1] = in.c[2];
      out.c[2] = in.c[1];
      out.c[3] = in.c[0];
      return out.f;
    }

#else /*__BIG_ENDIAN__*/

    inline int   DDSLittleLong(int src) { return src; }
    inline short int DDSLittleShort(short int src) { return src; }
    inline float DDSLittleFloat(float src) { return src; }

    inline int DDSBigLong(int src)
    {
      return ((src & 0xFF000000) >> 24) |
        ((src & 0x00FF0000) >> 8) |
        ((src & 0x0000FF00) << 8) |
        ((src & 0x000000FF) << 24);
    }

    inline short int DDSBigShort(short int src)
    {
      return ((src & 0xFF00) >> 8) |
        ((src & 0x00FF) << 8);
    }

    inline float DDSBigFloat(float src)
    {
      floatSwapUnion in, out;
      in.f = src;
      out.c[0] = in.c[3];
      out.c[1] = in.c[2];
      out.c[2] = in.c[1];
      out.c[3] = in.c[0];
      return out.f;
    }

#endif /*__BIG_ENDIAN__*/


    /*!
    Surface Loader for DDS images
    */
    class CImageLoaderDDS
    {
    private:
      static
        /*
        DDSGetInfo()
        extracts relevant info from a dds texture, returns 0 on success
        */
        int DDSGetInfo(ddsHeader* dds, int* width, int* height, ECOLOR_FORMAT* pf, bool &extendeddx10)
      {
        /* dummy test */
        if (dds == NULL)
          return -1;

        /* test dds header */
        if (DDSLittleLong(dds->Size) != 124)
          return -1;
        if (!(DDSLittleLong(dds->Flags) & DDSD_PIXELFORMAT))
          return -1;
        if (!(DDSLittleLong(dds->Flags) & DDSD_CAPS))
          return -1;

        /* extract width and height */
        if (width != NULL)
          *width = DDSLittleLong(dds->Width);
        if (height != NULL)
          *height = DDSLittleLong(dds->Height);

        /* get pixel format */

        /* extract fourCC */
        const unsigned fourCC = dds->PixelFormat.FourCC;
        *pf = ECF_UNKNOWN;
        extendeddx10 = false;

        /* test it */
        if (fourCC == 0)
          *pf = ECF_A8R8G8B8;
        if (fourCC == (*(unsigned *)"DX10"))
          extendeddx10 = true;

        /* return ok */
        return 0;
      }

      static int DDSDXT10GetInfo(ddsHeaderDXT10* dds, ECOLOR_FORMAT* pf)
      {
        // Code here https://msdn.microsoft.com/en-us/library/windows/desktop/bb173059%28v=vs.85%29.aspx
        unsigned int format = dds->format;
        if (format == 2)
          *pf = ECF_R32G32B32A32F;
        if (format == 10)
          *pf = ECF_R16G16B16A16F;
        if (format == 28)
          *pf = ECF_R8G8B8A8_UNORM;
        if (format == 71)
          *pf = ECF_BC1_UNORM;
        if (format == 72)
          *pf = ECF_BC1_UNORM_SRGB;
        if (format == 74)
          *pf = ECF_BC2_UNORM;
        if (format == 75)
          *pf = ECF_BC2_UNORM_SRGB;
        if (format == 77)
          *pf = ECF_BC3_UNORM;
        if (format == 78)
          *pf = ECF_BC3_UNORM_SRGB;
        if (format == 80)
          *pf = ECF_BC4_UNORM;
        if (format == 81)
          *pf = ECF_BC4_SNORM;
        if (format == 83)
          *pf = ECF_BC5_UNORM;
        if (format == 84)
          *pf = ECF_BC5_SNORM;
        return 0;
      }

      ddsHeader header;
      ddsHeaderDXT10 ddsheaderdxt10;
      ECOLOR_FORMAT format;
      size_t layersCount;
      bool is3D = false;
      bool isCubemap = false;
      bool useAlpha = false;
      unsigned mipMapCount;

      void loadMipLevels(std::ifstream &file)
      {
        if (!isCompressed(format)) // Uncompressed formats
        {
          size_t byteCount = formatBitCount(format) / 8;
          for (unsigned layer = 0; layer < layersCount; layer++)
          {
            size_t curWidth = header.Width;
            size_t curHeight = header.Height;

            std::vector<PackedMipMapLevel> MipmapData;
            for (unsigned i = 0; i < mipMapCount; i++)
            {
              size_t size = curWidth * curHeight * byteCount;
              char* data = new char[size];
              file.read(data, size);
              struct PackedMipMapLevel mipdata = { curWidth, curHeight, data, size };
              MipmapData.push_back(mipdata);

              if (curWidth > 1)
                curWidth >>= 1;

              if (curHeight > 1)
                curHeight >>= 1;
            }
            LoadedImage.Layers.push_back(MipmapData);
          }

          /*          if (header.Flags & DDSD_PITCH)
          dataSize = (unsigned)offset * header.Depth * (header.PixelFormat.RGBBitCount / 8);
          else
          dataSize = (unsigned)header.Width * header.Height * header.Depth * (header.PixelFormat.RGBBitCount / 8);*/

          switch (header.PixelFormat.RGBBitCount) // Bytes per pixel
          {
          case 16:
          {
            if (useAlpha)
            {
              if (header.PixelFormat.ABitMask == 0x8000)
                format = ECF_A1R5G5B5;
            }
            else
            {
              if (header.PixelFormat.RBitMask == 0xf800)
                format = ECF_R5G6B5;
            }

            break;
          }
          case 24:
          {
            if (!useAlpha)
            {
              if (header.PixelFormat.RBitMask == 0xff0000)
                format = ECF_R8G8B8;
            }

            break;
          }
          case 32:
          {
            if (useAlpha)
            {
              if (header.PixelFormat.RBitMask & 0xff0000)
                format = ECF_A8R8G8B8;
              else if (header.PixelFormat.RBitMask & 0xff)
              {
                // convert from A8B8G8R8 to A8R8G8B8
                unsigned char tmp = 0;

                /*                for (unsigned i = 0; i < dataSize; i += 4)
                {
                tmp = data[i];
                data[i] = data[i + 2];
                data[i + 2] = tmp;
                }*/
              }
            }
            break;
          }
          }
        }
        else // Compressed formats
        {
          unsigned block_width, block_height;
          unsigned block_size;
          switch (format)
          {
          case ECF_BC1_UNORM:
          case ECF_BC1_UNORM_SRGB:
            block_width = 4;
            block_height = 4;
            block_size = 8;
            break;
          case ECF_BC2_UNORM:
          case ECF_BC2_UNORM_SRGB:
          case ECF_BC3_UNORM:
          case ECF_BC3_UNORM_SRGB:
            block_width = 4;
            block_height = 4;
            block_size = 16;
            break;
          case ECF_BC4_UNORM:
          case ECF_BC4_SNORM:
          case ECF_BC5_UNORM:
          case ECF_BC5_SNORM:
            block_width = 4;
            block_height = 4;
            block_size = 16;
          default:
            abort();
          }

          for (unsigned layer = 0; layer < layersCount; layer++)
          {
            unsigned curWidth = header.Width;
            unsigned curHeight = header.Height;
            std::vector<PackedMipMapLevel> MipmapData;
            for (unsigned i = 0; i < mipMapCount; i++)
            {
              unsigned size = ((curWidth + block_width - 1) / block_width) * ((curHeight + block_height - 1) / block_height) * block_size;
              char* data = new char[size];
              file.read(data, size);
              struct PackedMipMapLevel mipdata = { curWidth, curHeight, data, size };
              MipmapData.push_back(mipdata);

              if (curWidth > 1)
                curWidth >>= 1;

              if (curHeight > 1)
                curHeight >>= 1;
            }
            LoadedImage.Layers.push_back(MipmapData);
          }
        }
      }

      IImage LoadedImage;
    public:
      //! returns true if the file maybe is able to be loaded by this class
      static bool isALoadableFileFormat(std::ifstream &f)
      {
        if (!f.is_open())
          return false;

        char MagicWord[4];
        f.read(MagicWord, 4);

        return (MagicWord[0] == 'D' && MagicWord[1] == 'D' && MagicWord[2] == 'S');
      }

      CImageLoaderDDS(std::ifstream &file) : format(ECF_UNKNOWN), mipMapCount(0)
      {
        if (!isALoadableFileFormat(file))
          abort();

        // Load DDS headers
        file.read((char*)&header, sizeof(ddsHeader));

        int width, height;
        bool extendeddx10;
        DDSGetInfo(&header, &width, &height, &format, extendeddx10);
        if (extendeddx10)
        {
          file.read((char*)&ddsheaderdxt10, sizeof(ddsHeaderDXT10));
          DDSDXT10GetInfo(&ddsheaderdxt10, &format);
        }

        is3D = header.Depth > 0 && (header.Flags & DDSD_DEPTH);
        isCubemap = header.Caps.caps2 & DDSCAPS2_CUBEMAP;

        layersCount = 1;
        if (isCubemap)
        {
          layersCount = 6;
          LoadedImage.Type = TextureType::CUBEMAP;
        }
        else
        {
          LoadedImage.Type = TextureType::TEXTURE2D;
        }
        assert(!is3D);
        if (!is3D)
          header.Depth = 1;

        useAlpha = header.PixelFormat.Flags & DDPF_ALPHAPIXELS;

        if (header.MipMapCount > 0 && (header.Flags & DDSD_MIPMAPCOUNT))
          mipMapCount = header.MipMapCount;

        LoadedImage.Layers.clear();
        loadMipLevels(file);

        LoadedImage.Format = format;
        file.close();
      }

      ~CImageLoaderDDS()
      {
        for (auto Layer : LoadedImage.Layers)
        {
          for (struct PackedMipMapLevel miplevel : Layer)
            delete[] miplevel.Data;
        }
      }

      const IImage& getLoadedImage() const
      {
        return LoadedImage;
      }
    };

  } // end namespace video
} // end namespace irr

#endif
