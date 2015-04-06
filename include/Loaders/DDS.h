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
#include <Loaders/IReadFile.h>
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

        /* dds pixel format types */
        enum eDDSPixelFormat
        {
            DDS_PF_ARGB8888,
            DDS_PF_DXT1,
            DDS_PF_DXT2,
            DDS_PF_DXT3,
            DDS_PF_DXT4,
            DDS_PF_DXT5,
            DDS_PF_UNKNOWN
        };

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
            char Magic[4];
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

        int   DDSLittleLong(int src) { return src; }
        short int DDSLittleShort(short int src) { return src; }
        float DDSLittleFloat(float src) { return src; }

        int DDSBigLong(int src)
        {
            return ((src & 0xFF000000) >> 24) |
                ((src & 0x00FF0000) >> 8) |
                ((src & 0x0000FF00) << 8) |
                ((src & 0x000000FF) << 24);
        }

        short int DDSBigShort(short int src)
        {
            return ((src & 0xFF00) >> 8) |
                ((src & 0x00FF) << 8);
        }

        float DDSBigFloat(float src)
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
                int DDSGetInfo(ddsHeader* dds, int* width, int* height, eDDSPixelFormat* pf, bool &extendeddx10)
            {
                /* dummy test */
                if (dds == NULL)
                    return -1;

                /* test dds header */
                if (*((int*)dds->Magic) != *((int*) "DDS "))
                    return -1;
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
                *pf = DDS_PF_UNKNOWN;
                extendeddx10 = false;

                /* test it */
                if (fourCC == 0)
                    *pf = DDS_PF_ARGB8888;
                if (fourCC == (*(unsigned *)"DX10"))
                    extendeddx10 = true;

                /* return ok */
                return 0;
            }

            static int DDSDXT10GetInfo(ddsHeaderDXT10* dds, eDDSPixelFormat* pf)
            {
                if (dds->format == 72)
                    *pf = DDS_PF_DXT1;
                return 0;
            }

        public:

            //! returns true if the file maybe is able to be loaded by this class
            //! based on the file extension (e.g. ".tga")
            //bool isALoadableFileExtension(const io::path& filename) const;

            //! returns true if the file maybe is able to be loaded by this class
            static bool isALoadableFileFormat(io::IReadFile* file)
            {
                if (!file)
                    return false;

                char MagicWord[4];
                file->read(&MagicWord, 4);

                return (MagicWord[0] == 'D' && MagicWord[1] == 'D' && MagicWord[2] == 'S');
            }

            //! creates a surface from the file
            static IImage* loadImage(io::IReadFile* file)
            {
                ddsHeader header;
                IImage* image = 0;
                int width, height;
                eDDSPixelFormat pixelFormat;
                ECOLOR_FORMAT format = ECF_UNKNOWN;
                unsigned dataSize = 0;
                bool is3D = false;
                bool useAlpha = false;
                unsigned mipMapCount = 0;

                file->seek(0);
                file->read(&header, sizeof(ddsHeader));

                bool extendeddx10;
                if (0 == DDSGetInfo(&header, &width, &height, &pixelFormat, extendeddx10))
                {
                    if (extendeddx10)
                    {
                        ddsHeaderDXT10 ddsheaderdxt10;
                        file->read(&ddsheaderdxt10, sizeof(ddsHeaderDXT10));
                        DDSDXT10GetInfo(&ddsheaderdxt10, &pixelFormat);
                    }

                    is3D = header.Depth > 0 && (header.Flags & DDSD_DEPTH);

                    if (!is3D)
                        header.Depth = 1;

                    useAlpha = header.PixelFormat.Flags & DDPF_ALPHAPIXELS;

                    if (header.MipMapCount > 0 && (header.Flags & DDSD_MIPMAPCOUNT))
                        mipMapCount = header.MipMapCount;

                    if (header.PixelFormat.Flags & DDPF_RGB) // Uncompressed formats
                    {
                        unsigned byteCount = header.PixelFormat.RGBBitCount / 8;

                        if (header.Flags & DDSD_PITCH)
                            dataSize = header.PitchOrLinearSize * header.Height * header.Depth * (header.PixelFormat.RGBBitCount / 8);
                        else
                            dataSize = header.Width * header.Height * header.Depth * (header.PixelFormat.RGBBitCount / 8);

                        unsigned char* data = new unsigned char[dataSize];
                        file->read(data, dataSize);

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

                                    for (unsigned i = 0; i < dataSize; i += 4)
                                    {
                                        tmp = data[i];
                                        data[i] = data[i + 2];
                                        data[i + 2] = tmp;
                                    }
                                }
                            }

                            break;
                        }
                        }

                        if (format != ECF_UNKNOWN)
                        {
                            if (!is3D) // Currently 3D textures are unsupported.
                            {
                                image = new IImage(format, header.Width, header.Height, header.PitchOrLinearSize, false);
                                memcpy(image->getPointer(), data, dataSize);
                            }
                        }
                        else
                        {
                            delete[] data;
                        }
                    }
                    else if (header.PixelFormat.Flags & DDPF_FOURCC) // Compressed formats
                    {
                        switch (pixelFormat)
                        {
                        case DDS_PF_DXT1:
                        {
                            unsigned curWidth = header.Width;
                            unsigned curHeight = header.Height;

                            dataSize = ((curWidth + 3) / 4) * ((curHeight + 3) / 4) * 8;

                            // MipMap
/*                            do
                            {
                                if (curWidth > 1)
                                    curWidth >>= 1;

                                if (curHeight > 1)
                                    curHeight >>= 1;

                                dataSize += ((curWidth + 3) / 4) * ((curHeight + 3) / 4) * 8;
                            } while (curWidth != 1 || curWidth != 1);*/

                            format = ECF_BC1;
                            break;
                        }
                        case DDS_PF_DXT2:
                        case DDS_PF_DXT3:
                        {
                            unsigned curWidth = header.Width;
                            unsigned curHeight = header.Height;

                            dataSize = ((curWidth + 3) / 4) * ((curHeight + 3) / 4) * 16;

                            do
                            {
                                if (curWidth > 1)
                                    curWidth >>= 1;

                                if (curHeight > 1)
                                    curHeight >>= 1;

                                dataSize += ((curWidth + 3) / 4) * ((curHeight + 3) / 4) * 16;
                            } while (curWidth != 1 || curWidth != 1);

                            format = ECF_BC3;
                            break;
                        }
                        case DDS_PF_DXT4:
                        case DDS_PF_DXT5:
                        {
                            unsigned curWidth = header.Width;
                            unsigned curHeight = header.Height;

                            dataSize = ((curWidth + 3) / 4) * ((curHeight + 3) / 4) * 16;

                            do
                            {
                                if (curWidth > 1)
                                    curWidth >>= 1;

                                if (curHeight > 1)
                                    curHeight >>= 1;

                                dataSize += ((curWidth + 3) / 4) * ((curHeight + 3) / 4) * 16;
                            } while (curWidth != 1 || curWidth != 1);

                            format = ECF_BC5;
                            break;
                        }
                        }

                        if (format != ECF_UNKNOWN)
                        {
                            if (!is3D) // Currently 3D textures are unsupported.
                            {
                                unsigned char* data = new unsigned char[dataSize];
                                file->read(data, dataSize);

                                bool hasMipMap = (mipMapCount > 0) ? true : false;

                                image = new IImage(format, header.Width, header.Height, header.PitchOrLinearSize, false);
                                image->ptrsz = dataSize;
                                memcpy(image->getPointer(), data, dataSize);
                            }
                        }
                    }
                }

                return image;
            }

        };


    } // end namespace video
} // end namespace irr



//#include "os.h"
//#include "CColorConverter.h"
//#include "CImage.h"
//#include "irrString.h"


namespace irr
{

    namespace video
    {


#ifdef _IRR_COMPILE_WITH_DDS_DECODER_LOADER_

        /*
        DDSDecompressARGB8888()
        decompresses an argb 8888 format texture
        */
        s32 DDSDecompressARGB8888(ddsHeader* dds, u8* data, s32 width, s32 height, u8* pixels)
        {
            /* setup */
            u8* in = data;
            u8* out = pixels;

            /* walk y */
            for (s32 y = 0; y < height; y++)
            {
                /* walk x */
                for (s32 x = 0; x < width; x++)
                {
                    *out++ = *in++;
                    *out++ = *in++;
                    *out++ = *in++;
                    *out++ = *in++;
                }
            }

            /* return ok */
            return 0;
        }


        /*!
        DDSGetColorBlockColors()
        extracts colors from a dds color block
        */
        void DDSGetColorBlockColors(ddsColorBlock* block, ddsColor colors[4])
        {
            u16		word;


            /* color 0 */
            word = DDSLittleShort(block->colors[0]);
            colors[0].a = 0xff;

            /* extract rgb bits */
            colors[0].b = (u8)word;
            colors[0].b <<= 3;
            colors[0].b |= (colors[0].b >> 5);
            word >>= 5;
            colors[0].g = (u8)word;
            colors[0].g <<= 2;
            colors[0].g |= (colors[0].g >> 5);
            word >>= 6;
            colors[0].r = (u8)word;
            colors[0].r <<= 3;
            colors[0].r |= (colors[0].r >> 5);

            /* same for color 1 */
            word = DDSLittleShort(block->colors[1]);
            colors[1].a = 0xff;

            /* extract rgb bits */
            colors[1].b = (u8)word;
            colors[1].b <<= 3;
            colors[1].b |= (colors[1].b >> 5);
            word >>= 5;
            colors[1].g = (u8)word;
            colors[1].g <<= 2;
            colors[1].g |= (colors[1].g >> 5);
            word >>= 6;
            colors[1].r = (u8)word;
            colors[1].r <<= 3;
            colors[1].r |= (colors[1].r >> 5);

            /* use this for all but the super-freak math method */
            if (block->colors[0] > block->colors[1])
            {
                /* four-color block: derive the other two colors.
                00 = color 0, 01 = color 1, 10 = color 2, 11 = color 3
                these two bit codes correspond to the 2-bit fields
                stored in the 64-bit block. */

                word = ((u16)colors[0].r * 2 + (u16)colors[1].r) / 3;
                /* no +1 for rounding */
                /* as bits have been shifted to 888 */
                colors[2].r = (u8)word;
                word = ((u16)colors[0].g * 2 + (u16)colors[1].g) / 3;
                colors[2].g = (u8)word;
                word = ((u16)colors[0].b * 2 + (u16)colors[1].b) / 3;
                colors[2].b = (u8)word;
                colors[2].a = 0xff;

                word = ((u16)colors[0].r + (u16)colors[1].r * 2) / 3;
                colors[3].r = (u8)word;
                word = ((u16)colors[0].g + (u16)colors[1].g * 2) / 3;
                colors[3].g = (u8)word;
                word = ((u16)colors[0].b + (u16)colors[1].b * 2) / 3;
                colors[3].b = (u8)word;
                colors[3].a = 0xff;
            }
            else
            {
                /* three-color block: derive the other color.
                00 = color 0, 01 = color 1, 10 = color 2,
                11 = transparent.
                These two bit codes correspond to the 2-bit fields
                stored in the 64-bit block */

                word = ((u16)colors[0].r + (u16)colors[1].r) / 2;
                colors[2].r = (u8)word;
                word = ((u16)colors[0].g + (u16)colors[1].g) / 2;
                colors[2].g = (u8)word;
                word = ((u16)colors[0].b + (u16)colors[1].b) / 2;
                colors[2].b = (u8)word;
                colors[2].a = 0xff;

                /* random color to indicate alpha */
                colors[3].r = 0x00;
                colors[3].g = 0xff;
                colors[3].b = 0xff;
                colors[3].a = 0x00;
            }
        }


        /*
        DDSDecodeColorBlock()
        decodes a dds color block
        fixme: make endian-safe
        */

        void DDSDecodeColorBlock(u32* pixel, ddsColorBlock* block, s32 width, u32 colors[4])
        {
            s32				r, n;
            u32	bits;
            u32	masks[] = { 3, 12, 3 << 4, 3 << 6 };	/* bit masks = 00000011, 00001100, 00110000, 11000000 */
            s32				shift[] = { 0, 2, 4, 6 };


            /* r steps through lines in y */
            for (r = 0; r < 4; r++, pixel += (width - 4))	/* no width * 4 as u32 ptr inc will * 4 */
            {
                /* width * 4 bytes per pixel per line, each j dxtc row is 4 lines of pixels */

                /* n steps through pixels */
                for (n = 0; n < 4; n++)
                {
                    bits = block->row[r] & masks[n];
                    bits >>= shift[n];

                    switch (bits)
                    {
                    case 0:
                        *pixel = colors[0];
                        pixel++;
                        break;

                    case 1:
                        *pixel = colors[1];
                        pixel++;
                        break;

                    case 2:
                        *pixel = colors[2];
                        pixel++;
                        break;

                    case 3:
                        *pixel = colors[3];
                        pixel++;
                        break;

                    default:
                        /* invalid */
                        pixel++;
                        break;
                    }
                }
            }
        }


        /*
        DDSDecodeAlphaExplicit()
        decodes a dds explicit alpha block
        */
        void DDSDecodeAlphaExplicit(u32* pixel, ddsAlphaBlockExplicit* alphaBlock, s32 width, u32 alphaZero)
        {
            s32				row, pix;
            u16	word;
            ddsColor		color;


            /* clear color */
            color.r = 0;
            color.g = 0;
            color.b = 0;

            /* walk rows */
            for (row = 0; row < 4; row++, pixel += (width - 4))
            {
                word = DDSLittleShort(alphaBlock->row[row]);

                /* walk pixels */
                for (pix = 0; pix < 4; pix++)
                {
                    /* zero the alpha bits of image pixel */
                    *pixel &= alphaZero;
                    color.a = word & 0x000F;
                    color.a = color.a | (color.a << 4);
                    *pixel |= *((u32*)&color);
                    word >>= 4;		/* move next bits to lowest 4 */
                    pixel++;		/* move to next pixel in the row */
                }
            }
        }



        /*
        DDSDecodeAlpha3BitLinear()
        decodes interpolated alpha block
        */
        void DDSDecodeAlpha3BitLinear(u32* pixel, ddsAlphaBlock3BitLinear* alphaBlock, s32 width, u32 alphaZero)
        {

            s32 row, pix;
            u32 stuff;
            u8 bits[4][4];
            u16 alphas[8];
            ddsColor aColors[4][4];

            /* get initial alphas */
            alphas[0] = alphaBlock->alpha0;
            alphas[1] = alphaBlock->alpha1;

            /* 8-alpha block */
            if (alphas[0] > alphas[1])
            {
                /* 000 = alpha_0, 001 = alpha_1, others are interpolated */
                alphas[2] = (6 * alphas[0] + alphas[1]) / 7;	/* bit code 010 */
                alphas[3] = (5 * alphas[0] + 2 * alphas[1]) / 7;	/* bit code 011 */
                alphas[4] = (4 * alphas[0] + 3 * alphas[1]) / 7;	/* bit code 100 */
                alphas[5] = (3 * alphas[0] + 4 * alphas[1]) / 7;	/* bit code 101 */
                alphas[6] = (2 * alphas[0] + 5 * alphas[1]) / 7;	/* bit code 110 */
                alphas[7] = (alphas[0] + 6 * alphas[1]) / 7;	/* bit code 111 */
            }

            /* 6-alpha block */
            else
            {
                /* 000 = alpha_0, 001 = alpha_1, others are interpolated */
                alphas[2] = (4 * alphas[0] + alphas[1]) / 5;	/* bit code 010 */
                alphas[3] = (3 * alphas[0] + 2 * alphas[1]) / 5;	/* bit code 011 */
                alphas[4] = (2 * alphas[0] + 3 * alphas[1]) / 5;	/* bit code 100 */
                alphas[5] = (alphas[0] + 4 * alphas[1]) / 5;	/* bit code 101 */
                alphas[6] = 0;										/* bit code 110 */
                alphas[7] = 255;										/* bit code 111 */
            }

            /* decode 3-bit fields into array of 16 bytes with same value */

            /* first two rows of 4 pixels each */
            stuff = *((u32*)&(alphaBlock->stuff[0]));

            bits[0][0] = (u8)(stuff & 0x00000007);
            stuff >>= 3;
            bits[0][1] = (u8)(stuff & 0x00000007);
            stuff >>= 3;
            bits[0][2] = (u8)(stuff & 0x00000007);
            stuff >>= 3;
            bits[0][3] = (u8)(stuff & 0x00000007);
            stuff >>= 3;
            bits[1][0] = (u8)(stuff & 0x00000007);
            stuff >>= 3;
            bits[1][1] = (u8)(stuff & 0x00000007);
            stuff >>= 3;
            bits[1][2] = (u8)(stuff & 0x00000007);
            stuff >>= 3;
            bits[1][3] = (u8)(stuff & 0x00000007);

            /* last two rows */
            stuff = *((u32*)&(alphaBlock->stuff[3])); /* last 3 bytes */

            bits[2][0] = (u8)(stuff & 0x00000007);
            stuff >>= 3;
            bits[2][1] = (u8)(stuff & 0x00000007);
            stuff >>= 3;
            bits[2][2] = (u8)(stuff & 0x00000007);
            stuff >>= 3;
            bits[2][3] = (u8)(stuff & 0x00000007);
            stuff >>= 3;
            bits[3][0] = (u8)(stuff & 0x00000007);
            stuff >>= 3;
            bits[3][1] = (u8)(stuff & 0x00000007);
            stuff >>= 3;
            bits[3][2] = (u8)(stuff & 0x00000007);
            stuff >>= 3;
            bits[3][3] = (u8)(stuff & 0x00000007);

            /* decode the codes into alpha values */
            for (row = 0; row < 4; row++)
            {
                for (pix = 0; pix < 4; pix++)
                {
                    aColors[row][pix].r = 0;
                    aColors[row][pix].g = 0;
                    aColors[row][pix].b = 0;
                    aColors[row][pix].a = (u8)alphas[bits[row][pix]];
                }
            }

            /* write out alpha values to the image bits */
            for (row = 0; row < 4; row++, pixel += width - 4)
            {
                for (pix = 0; pix < 4; pix++)
                {
                    /* zero the alpha bits of image pixel */
                    *pixel &= alphaZero;

                    /* or the bits into the prev. nulled alpha */
                    *pixel |= *((u32*)&(aColors[row][pix]));
                    pixel++;
                }
            }
        }


        /*
        DDSDecompressDXT1()
        decompresses a dxt1 format texture
        */
        s32 DDSDecompressDXT1(ddsHeader* dds, u8* data, s32 width, s32 height, u8* pixels)
        {
            s32 x, y, xBlocks, yBlocks;
            u32 *pixel;
            ddsColorBlock *block;
            ddsColor colors[4];

            /* setup */
            xBlocks = width / 4;
            yBlocks = height / 4;

            /* walk y */
            for (y = 0; y < yBlocks; y++)
            {
                /* 8 bytes per block */
                block = (ddsColorBlock*)(data + y * xBlocks * 8);

                /* walk x */
                for (x = 0; x < xBlocks; x++, block++)
                {
                    DDSGetColorBlockColors(block, colors);
                    pixel = (u32*)(pixels + x * 16 + (y * 4) * width * 4);
                    DDSDecodeColorBlock(pixel, block, width, (u32*)colors);
                }
            }

            /* return ok */
            return 0;
        }


        /*
        DDSDecompressDXT3()
        decompresses a dxt3 format texture
        */

        s32 DDSDecompressDXT3(ddsHeader* dds, u8* data, s32 width, s32 height, u8* pixels)
        {
            s32 x, y, xBlocks, yBlocks;
            u32 *pixel, alphaZero;
            ddsColorBlock *block;
            ddsAlphaBlockExplicit *alphaBlock;
            ddsColor colors[4];

            /* setup */
            xBlocks = width / 4;
            yBlocks = height / 4;

            /* create zero alpha */
            colors[0].a = 0;
            colors[0].r = 0xFF;
            colors[0].g = 0xFF;
            colors[0].b = 0xFF;
            alphaZero = *((u32*)&colors[0]);

            /* walk y */
            for (y = 0; y < yBlocks; y++)
            {
                /* 8 bytes per block, 1 block for alpha, 1 block for color */
                block = (ddsColorBlock*)(data + y * xBlocks * 16);

                /* walk x */
                for (x = 0; x < xBlocks; x++, block++)
                {
                    /* get alpha block */
                    alphaBlock = (ddsAlphaBlockExplicit*)block;

                    /* get color block */
                    block++;
                    DDSGetColorBlockColors(block, colors);

                    /* decode color block */
                    pixel = (u32*)(pixels + x * 16 + (y * 4) * width * 4);
                    DDSDecodeColorBlock(pixel, block, width, (u32*)colors);

                    /* overwrite alpha bits with alpha block */
                    DDSDecodeAlphaExplicit(pixel, alphaBlock, width, alphaZero);
                }
            }

            /* return ok */
            return 0;
        }


        /*
        DDSDecompressDXT5()
        decompresses a dxt5 format texture
        */
        s32 DDSDecompressDXT5(ddsHeader* dds, u8* data, s32 width, s32 height, u8* pixels)
        {
            s32 x, y, xBlocks, yBlocks;
            u32 *pixel, alphaZero;
            ddsColorBlock *block;
            ddsAlphaBlock3BitLinear *alphaBlock;
            ddsColor colors[4];

            /* setup */
            xBlocks = width / 4;
            yBlocks = height / 4;

            /* create zero alpha */
            colors[0].a = 0;
            colors[0].r = 0xFF;
            colors[0].g = 0xFF;
            colors[0].b = 0xFF;
            alphaZero = *((u32*)&colors[0]);

            /* walk y */
            for (y = 0; y < yBlocks; y++)
            {
                /* 8 bytes per block, 1 block for alpha, 1 block for color */
                block = (ddsColorBlock*)(data + y * xBlocks * 16);

                /* walk x */
                for (x = 0; x < xBlocks; x++, block++)
                {
                    /* get alpha block */
                    alphaBlock = (ddsAlphaBlock3BitLinear*)block;

                    /* get color block */
                    block++;
                    DDSGetColorBlockColors(block, colors);

                    /* decode color block */
                    pixel = (u32*)(pixels + x * 16 + (y * 4) * width * 4);
                    DDSDecodeColorBlock(pixel, block, width, (u32*)colors);

                    /* overwrite alpha bits with alpha block */
                    DDSDecodeAlpha3BitLinear(pixel, alphaBlock, width, alphaZero);
                }
            }

            /* return ok */
            return 0;
        }


        /*
        DDSDecompressDXT2()
        decompresses a dxt2 format texture (fixme: un-premultiply alpha)
        */
        s32 DDSDecompressDXT2(ddsHeader* dds, u8* data, s32 width, s32 height, u8* pixels)
        {
            /* decompress dxt3 first */
            const s32 r = DDSDecompressDXT3(dds, data, width, height, pixels);

            /* return to sender */
            return r;
        }


        /*
        DDSDecompressDXT4()
        decompresses a dxt4 format texture (fixme: un-premultiply alpha)
        */
        s32 DDSDecompressDXT4(ddsHeader* dds, u8* data, s32 width, s32 height, u8* pixels)
        {
            /* decompress dxt5 first */
            const s32 r = DDSDecompressDXT5(dds, data, width, height, pixels);

            /* return to sender */
            return r;
        }


        /*
        DDSDecompress()
        decompresses a dds texture into an rgba image buffer, returns 0 on success
        */
        s32 DDSDecompress(ddsHeader* dds, u8* data, u8* pixels)
        {
            s32 width, height;
            eDDSPixelFormat pf;

            /* get dds info */
            s32 r = DDSGetInfo(dds, &width, &height, &pf);
            if (r)
                return r;

            /* decompress */
            switch (pf)
            {
            case DDS_PF_ARGB8888:
                /* fixme: support other [a]rgb formats */
                r = DDSDecompressARGB8888(dds, data, width, height, pixels);
                break;

            case DDS_PF_DXT1:
                r = DDSDecompressDXT1(dds, data, width, height, pixels);
                break;

            case DDS_PF_DXT2:
                r = DDSDecompressDXT2(dds, data, width, height, pixels);
                break;

            case DDS_PF_DXT3:
                r = DDSDecompressDXT3(dds, data, width, height, pixels);
                break;

            case DDS_PF_DXT4:
                r = DDSDecompressDXT4(dds, data, width, height, pixels);
                break;

            case DDS_PF_DXT5:
                r = DDSDecompressDXT5(dds, data, width, height, pixels);
                break;

            default: // DDS_PF_UNKNOWN
                r = -1;
                break;
            }

            /* return to sender */
            return r;
        }

#endif

    } // end namespace video
} // end namespace irr

#endif
