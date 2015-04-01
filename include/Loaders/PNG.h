// Copyright (C) 2002-2012 Nikolaus Gebhardt
// Copyright (C) 2015 Vincent Lejeune
// Contains code from the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __C_IMAGE_LOADER_PNG_H_INCLUDED__
#define __C_IMAGE_LOADER_PNG_H_INCLUDED__

#include <Core/IImage.h>
#include <Loaders/IReadFile.h>
#include <png.h>

namespace irr
{
  namespace video
  {
    // PNG function for file reading
    static void PNGAPI user_read_data_fcn(png_structp png_ptr, png_bytep data, png_size_t length)
    {
      png_size_t check;

      // changed by zola {
      io::IReadFile* file = (io::IReadFile*)png_get_io_ptr(png_ptr);
      check = (png_size_t)file->read((void*)data, (unsigned int)length);
      // }

      if (check != length)
        png_error(png_ptr, "Read Error");
    }

    //!  Surface Loader for PNG files
    class CImageLoaderPng
    {
    private:
      // PNG function for error handling
      static void png_cpexcept_error(png_structp png_ptr, png_const_charp msg)
      {
        printf("PNG fatal error", msg);
        longjmp(png_jmpbuf(png_ptr), 1);
      }

      // PNG function for warning handling
      static void png_cpexcept_warn(png_structp png_ptr, png_const_charp msg)
      {
        printf("PNG warning", msg);
      }

    public:

      //! returns true if the file maybe is able to be loaded by this class
      //! based on the file extension (e.g. ".png")
//      virtual bool isALoadableFileExtension(const io::path& filename) const;

      //! returns true if the file maybe is able to be loaded by this class
      bool isALoadableFileFormat(io::IReadFile* file) const
      {
        if (!file)
          return false;

        png_byte buffer[8];
        // Read the first few bytes of the PNG file
        if (file->read(buffer, 8) != 8)
          return false;

        // Check if it really is a PNG file
        return !png_sig_cmp(buffer, 0, 8);
      }

      //! creates a surface from the file
      IImage* loadImage(io::IReadFile* file) const
      {
        if (!file)
          return 0;

        bool sRGB = false;
        IImage* image = 0;
        //Used to point to image rows
        unsigned char** RowPointers = 0;

        png_byte buffer[8];
        // Read the first few bytes of the PNG file
        if (file->read(buffer, 8) != 8)
        {
          printf("LOAD PNG: can't read file\n", file->getFileName());
          return 0;
        }

        // Check if it really is a PNG file
        if (png_sig_cmp(buffer, 0, 8))
        {
          printf("LOAD PNG: not really a png\n", file->getFileName());
          return 0;
        }

        // Allocate the png read struct
        png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
          NULL, (png_error_ptr)png_cpexcept_error, (png_error_ptr)png_cpexcept_warn);
        if (!png_ptr)
        {
          printf("LOAD PNG: Internal PNG create read struct failure\n", file->getFileName());
          return 0;
        }

        // Allocate the png info struct
        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
        {
          printf("LOAD PNG: Internal PNG create info struct failure\n", file->getFileName());
          png_destroy_read_struct(&png_ptr, NULL, NULL);
          return 0;
        }

        // for proper error handling
        if (setjmp(png_jmpbuf(png_ptr)))
        {
          png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
          if (RowPointers)
            delete[] RowPointers;
          return 0;
        }

        // changed by zola so we don't need to have public FILE pointers
        png_set_read_fn(png_ptr, file, user_read_data_fcn);

        png_set_sig_bytes(png_ptr, 8); // Tell png that we read the signature

        png_read_info(png_ptr, info_ptr); // Read the info section of the png file

        unsigned int Width;
        unsigned int Height;
        int BitDepth;
        int ColorType;
        {
          // Use temporary variables to avoid passing casted pointers
          png_uint_32 w, h;
          // Extract info
          png_get_IHDR(png_ptr, info_ptr,
            &w, &h,
            &BitDepth, &ColorType, NULL, NULL, NULL);
          Width = w;
          Height = h;
        }

        // Convert palette color to true color
        if (ColorType == PNG_COLOR_TYPE_PALETTE)
          png_set_palette_to_rgb(png_ptr);

        // Convert low bit colors to 8 bit colors
        if (BitDepth < 8)
        {
          if (ColorType == PNG_COLOR_TYPE_GRAY || ColorType == PNG_COLOR_TYPE_GRAY_ALPHA)
            png_set_expand_gray_1_2_4_to_8(png_ptr);
          else
            png_set_packing(png_ptr);
        }

        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
          png_set_tRNS_to_alpha(png_ptr);

        // Convert high bit colors to 8 bit colors
        if (BitDepth == 16)
          png_set_strip_16(png_ptr);

        // Convert gray color to true color
        if (ColorType == PNG_COLOR_TYPE_GRAY || ColorType == PNG_COLOR_TYPE_GRAY_ALPHA)
          png_set_gray_to_rgb(png_ptr);

        int intent;
        const double screen_gamma = 2.2;

        if (png_get_sRGB(png_ptr, info_ptr, &intent))
        {
          sRGB = true;
          png_set_gamma(png_ptr, screen_gamma, 0.45455);
        }
        else
        {
          double image_gamma;
          if (png_get_gAMA(png_ptr, info_ptr, &image_gamma))
            png_set_gamma(png_ptr, screen_gamma, image_gamma);
          else
            png_set_gamma(png_ptr, screen_gamma, 0.45455);
        }

        // Update the changes in between, as we need to get the new color type
        // for proper processing of the RGBA type
        png_read_update_info(png_ptr, info_ptr);
        {
          // Use temporary variables to avoid passing casted pointers
          png_uint_32 w, h;
          // Extract info
          png_get_IHDR(png_ptr, info_ptr,
            &w, &h,
            &BitDepth, &ColorType, NULL, NULL, NULL);
          Width = w;
          Height = h;
        }

        // Convert RGBA to BGRA
        if (ColorType == PNG_COLOR_TYPE_RGB_ALPHA)
        {
#ifdef __BIG_ENDIAN__
          png_set_swap_alpha(png_ptr);
#else
          png_set_bgr(png_ptr);
#endif
        }

        // Create the image structure to be filled by png data
        if (ColorType == PNG_COLOR_TYPE_RGB_ALPHA)
          image = new IImage(ECF_A8R8G8B8, Width, Height, sRGB);
        else
          image = new IImage(ECF_R8G8B8, Width, Height, sRGB);
        if (!image)
        {
          printf("LOAD PNG: Internal PNG create image struct failure\n", file->getFileName());
          png_destroy_read_struct(&png_ptr, NULL, NULL);
          return 0;
        }

        // Create array of pointers to rows in image data
        RowPointers = new png_bytep[Height];
        if (!RowPointers)
        {
          printf("LOAD PNG: Internal PNG create row pointers failure\n", file->getFileName());
          png_destroy_read_struct(&png_ptr, NULL, NULL);
          delete image;
          return 0;
        }

        // Fill array of pointers to rows in image data
        unsigned char* data = (unsigned char*)image->getPointer();
        for (unsigned int i = 0; i<Height; ++i)
        {
          RowPointers[i] = data;
          data += image->getPitch();
        }

        // for proper error handling
        if (setjmp(png_jmpbuf(png_ptr)))
        {
          png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
          delete[] RowPointers;
          delete image;
          return 0;
        }

        // Read data using the library function that handles all transformations including interlacing
        png_read_image(png_ptr, RowPointers);

        png_read_end(png_ptr, NULL);
        delete[] RowPointers;
        png_destroy_read_struct(&png_ptr, &info_ptr, 0); // Clean up memory

        return image;
      }
    };
  } // end namespace video
} // end namespace irr

#endif

