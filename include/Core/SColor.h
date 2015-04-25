// Copyright (C) 2002-2012 Nikolaus Gebhardt
// Copyright (C) 2015 Vincent Lejeune
// Contains code from the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __COLOR_H_INCLUDED__
#define __COLOR_H_INCLUDED__

#include <cmath>
#include <cassert>

namespace irr
{
  namespace video
  {
    //! An enum for the color format of textures used by the Irrlicht Engine.
    /** A color format specifies how color information is stored. */
    enum ECOLOR_FORMAT
    {
      //! 16 bit color format used by the software driver.
      /** It is thus preferred by all other irrlicht engine video drivers.
      There are 5 bits for every color component, and a single bit is left
      for alpha information. */
      ECF_A1R5G5B5 = 0,

      //! Standard 16 bit color format.
      ECF_R5G6B5,

      //! 24 bit color, no alpha channel, but 8 bit for red, green and blue.
      ECF_R8G8B8,

      //! Default 32 bit color format. 8 bits are used for every component: red, green, blue and alpha.
      ECF_A8R8G8B8,
      ECF_R8G8B8A8_UNORM,
      ECF_R8G8B8A8_UNORM_SRGB,

      //! The normalized non-float formats from the _rg extension
      ECF_R8,
      ECF_R8G8,
      ECF_R16,
      ECF_R16G16,

      /** Floating Point formats. The following formats may only be used for render target textures. */

      //! 16 bit floating point format using 16 bits for the red channel.
      ECF_R16F,

      //! 32 bit floating point format using 16 bits for the red channel and 16 bits for the green channel.
      ECF_G16R16F,

      //! 64 bit floating point format 16 bits are used for the red, green, blue and alpha channels.
      ECF_R16G16B16A16F,

      //! 32 bit floating point format using 32 bits for the red channel.
      ECF_R32F,

      //! 64 bit floating point format using 32 bits for the red channel and 32 bits for the green channel.
      ECF_G32R32F,

      //! 128 bit floating point format. 32 bits are used for the red, green, blue and alpha channels.
      ECF_R32G32B32A32F,


      // Block Compressed format
      ECF_BC1_UNORM, ECF_BC1_UNORM_SRGB,
      ECF_BC2_UNORM, ECF_BC2_UNORM_SRGB,
      ECF_BC3_UNORM, ECF_BC3_UNORM_SRGB,
      ECF_BC4_UNORM, ECF_BC4_SNORM,
      ECF_BC5_UNORM, ECF_BC5_SNORM,


      //! Unknown color format:
      ECF_UNKNOWN
    };

    inline bool isCompressed(ECOLOR_FORMAT format)
    {
        switch (format)
        {
        default:
            return false;
        case ECF_BC1_UNORM:
        case ECF_BC1_UNORM_SRGB:
        case ECF_BC2_UNORM:
        case ECF_BC2_UNORM_SRGB:
        case ECF_BC3_UNORM:
        case ECF_BC3_UNORM_SRGB:
        case ECF_BC4_UNORM:
        case ECF_BC4_SNORM:
        case ECF_BC5_UNORM:
        case ECF_BC5_SNORM:
            return true;
        }
    }

    inline size_t formatBitCount(ECOLOR_FORMAT format)
    {
      assert(!isCompressed(format));
      switch (format)
      {
      case ECF_A8R8G8B8:
      case ECF_R8G8B8A8_UNORM:
      case ECF_R8G8B8A8_UNORM_SRGB:
        return 32;
      case ECF_R16G16B16A16F:
        return 64;
      case ECF_R32G32B32A32F:
        return 128;
      default:
        return 0;
      }
    }


    //! Creates a 16 bit A1R5G5B5 color
    inline short RGBA16(unsigned r, unsigned g, unsigned b, unsigned a = 0xFF)
    {
      return (short)((a & 0x80) << 8 |
        (r & 0xF8) << 7 |
        (g & 0xF8) << 2 |
        (b & 0xF8) >> 3);
    }


    //! Creates a 16 bit A1R5G5B5 color
    inline short RGB16(unsigned r, unsigned g, unsigned b)
    {
      return RGBA16(r, g, b);
    }


    //! Creates a 16bit A1R5G5B5 color, based on 16bit input values
    inline short RGB16from16(short r, short g, short b)
    {
      return (0x8000 |
        (r & 0x1F) << 10 |
        (g & 0x1F) << 5 |
        (b & 0x1F));
    }


    //! Converts a 32bit (X8R8G8B8) color to a 16bit A1R5G5B5 color
    inline short X8R8G8B8toA1R5G5B5(unsigned color)
    {
      return (short)(0x8000 |
        (color & 0x00F80000) >> 9 |
        (color & 0x0000F800) >> 6 |
        (color & 0x000000F8) >> 3);
    }


    //! Converts a 32bit (A8R8G8B8) color to a 16bit A1R5G5B5 color
    inline short A8R8G8B8toA1R5G5B5(unsigned color)
    {
      return (short)((color & 0x80000000) >> 16 |
        (color & 0x00F80000) >> 9 |
        (color & 0x0000F800) >> 6 |
        (color & 0x000000F8) >> 3);
    }


    //! Converts a 32bit (A8R8G8B8) color to a 16bit R5G6B5 color
    inline short A8R8G8B8toR5G6B5(unsigned color)
    {
      return (short)((color & 0x00F80000) >> 8 |
        (color & 0x0000FC00) >> 5 |
        (color & 0x000000F8) >> 3);
    }


    //! Convert A8R8G8B8 Color from A1R5G5B5 color
    /** build a nicer 32bit Color by extending dest lower bits with source high bits. */
    inline unsigned A1R5G5B5toA8R8G8B8(short color)
    {
      return (((-((int)color & 0x00008000) >> (int)31) & 0xFF000000) |
        ((color & 0x00007C00) << 9) | ((color & 0x00007000) << 4) |
        ((color & 0x000003E0) << 6) | ((color & 0x00000380) << 1) |
        ((color & 0x0000001F) << 3) | ((color & 0x0000001C) >> 2)
        );
    }


    //! Returns A8R8G8B8 Color from R5G6B5 color
    inline unsigned R5G6B5toA8R8G8B8(short color)
    {
      return 0xFF000000 |
        ((color & 0xF800) << 8) |
        ((color & 0x07E0) << 5) |
        ((color & 0x001F) << 3);
    }


    //! Returns A1R5G5B5 Color from R5G6B5 color
    inline short R5G6B5toA1R5G5B5(short color)
    {
      return 0x8000 | (((color & 0xFFC0) >> 1) | (color & 0x1F));
    }


    //! Returns R5G6B5 Color from A1R5G5B5 color
    inline short A1R5G5B5toR5G6B5(short color)
    {
      return (((color & 0x7FE0) << 1) | (color & 0x1F));
    }



    //! Returns the alpha component from A1R5G5B5 color
    /** In Irrlicht, alpha refers to opacity.
    \return The alpha value of the color. 0 is transparent, 1 is opaque. */
    inline unsigned getAlpha(short color)
    {
      return ((color >> 15) & 0x1);
    }


    //! Returns the red component from A1R5G5B5 color.
    /** Shift left by 3 to get 8 bit value. */
    inline unsigned getRed(short color)
    {
      return ((color >> 10) & 0x1F);
    }


    //! Returns the green component from A1R5G5B5 color
    /** Shift left by 3 to get 8 bit value. */
    inline unsigned getGreen(short color)
    {
      return ((color >> 5) & 0x1F);
    }


    //! Returns the blue component from A1R5G5B5 color
    /** Shift left by 3 to get 8 bit value. */
    inline unsigned getBlue(short color)
    {
      return (color & 0x1F);
    }


    //! Returns the average from a 16 bit A1R5G5B5 color
    /*	inline int getAverage(s16 color)
      {
      return ((getRed(color)<<3) + (getGreen(color)<<3) + (getBlue(color)<<3)) / 3;
      }*/


    //! Class representing a 32 bit ARGB color.
    /** The color values for alpha, red, green, and blue are
    stored in a single unsigned. So all four values may be between 0 and 255.
    Alpha in Irrlicht is opacity, so 0 is fully transparent, 255 is fully opaque (solid).
    This class is used by most parts of the Irrlicht Engine
    to specify a color. Another way is using the class SColorf, which
    stores the color values in 4 floats.
    This class must consist of only one unsigned and must not use virtual functions.
    */
    class SColor
    {
    public:

      //! Constructor of the Color. Does nothing.
      /** The color value is not initialized to save time. */
      SColor() {}

      //! Constructs the color from 4 values representing the alpha, red, green and blue component.
      /** Must be values between 0 and 255. */
      SColor(unsigned a, unsigned r, unsigned g, unsigned b)
        : color(((a & 0xff) << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff)) {}

      //! Constructs the color from a 32 bit value. Could be another color.
      SColor(unsigned clr)
        : color(clr) {}

      //! Returns the alpha component of the color.
      /** The alpha component defines how opaque a color is.
      \return The alpha value of the color. 0 is fully transparent, 255 is fully opaque. */
      unsigned getAlpha() const { return color >> 24; }

      //! Returns the red component of the color.
      /** \return Value between 0 and 255, specifying how red the color is.
      0 means no red, 255 means full red. */
      unsigned getRed() const { return (color >> 16) & 0xff; }

      //! Returns the green component of the color.
      /** \return Value between 0 and 255, specifying how green the color is.
      0 means no green, 255 means full green. */
      unsigned getGreen() const { return (color >> 8) & 0xff; }

      //! Returns the blue component of the color.
      /** \return Value between 0 and 255, specifying how blue the color is.
      0 means no blue, 255 means full blue. */
      unsigned getBlue() const { return color & 0xff; }

      //! Get lightness of the color in the range [0,255]
      /*		float getLightness() const
          {
          return 0.5f*(core::max_(core::max_(getRed(),getGreen()),getBlue())+core::min_(core::min_(getRed(),getGreen()),getBlue()));
          }*/

      //! Get luminance of the color in the range [0,255].
      float getLuminance() const
      {
        return 0.3f*getRed() + 0.59f*getGreen() + 0.11f*getBlue();
      }

      //! Get average intensity of the color in the range [0,255].
      unsigned getAverage() const
      {
        return (getRed() + getGreen() + getBlue()) / 3;
      }

      //! Sets the alpha component of the Color.
      /** The alpha component defines how transparent a color should be.
      \param a The alpha value of the color. 0 is fully transparent, 255 is fully opaque. */
      void setAlpha(unsigned a) { color = ((a & 0xff) << 24) | (color & 0x00ffffff); }

      //! Sets the red component of the Color.
      /** \param r: Has to be a value between 0 and 255.
      0 means no red, 255 means full red. */
      void setRed(unsigned r) { color = ((r & 0xff) << 16) | (color & 0xff00ffff); }

      //! Sets the green component of the Color.
      /** \param g: Has to be a value between 0 and 255.
      0 means no green, 255 means full green. */
      void setGreen(unsigned g) { color = ((g & 0xff) << 8) | (color & 0xffff00ff); }

      //! Sets the blue component of the Color.
      /** \param b: Has to be a value between 0 and 255.
      0 means no blue, 255 means full blue. */
      void setBlue(unsigned b) { color = (b & 0xff) | (color & 0xffffff00); }

      //! Calculates a 16 bit A1R5G5B5 value of this color.
      /** \return 16 bit A1R5G5B5 value of this color. */
      short toA1R5G5B5() const { return A8R8G8B8toA1R5G5B5(color); }

      //! Converts color to OpenGL color format
      /** From ARGB to RGBA in 4 byte components for endian aware
      passing to OpenGL
      \param dest: address where the 4x8 bit OpenGL color is stored. */
      void toOpenGLColor(char* dest) const
      {
        *dest = (char)getRed();
        *++dest = (char)getGreen();
        *++dest = (char)getBlue();
        *++dest = (char)getAlpha();
      }

      //! Sets all four components of the color at once.
      /** Constructs the color from 4 values representing the alpha,
      red, green and blue components of the color. Must be values
      between 0 and 255.
      \param a: Alpha component of the color. The alpha component
      defines how transparent a color should be. Has to be a value
      between 0 and 255. 255 means not transparent (opaque), 0 means
      fully transparent.
      \param r: Sets the red component of the Color. Has to be a
      value between 0 and 255. 0 means no red, 255 means full red.
      \param g: Sets the green component of the Color. Has to be a
      value between 0 and 255. 0 means no green, 255 means full
      green.
      \param b: Sets the blue component of the Color. Has to be a
      value between 0 and 255. 0 means no blue, 255 means full blue. */
      void set(unsigned a, unsigned r, unsigned g, unsigned b)
      {
        color = (((a & 0xff) << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff));
      }
      void set(unsigned col) { color = col; }

      //! Compares the color to another color.
      /** \return True if the colors are the same, and false if not. */
      bool operator==(const SColor& other) const { return other.color == color; }

      //! Compares the color to another color.
      /** \return True if the colors are different, and false if they are the same. */
      bool operator!=(const SColor& other) const { return other.color != color; }

      //! comparison operator
      /** \return True if this color is smaller than the other one */
      bool operator<(const SColor& other) const { return (color < other.color); }

      //! Adds two colors, result is clamped to 0..255 values
      /** \param other Color to add to this color
      \return Addition of the two colors, clamped to 0..255 values */
      /*		SColor operator+(const SColor& other) const
          {
          return SColor(core::min_(getAlpha() + other.getAlpha(), 255u),
          core::min_(getRed() + other.getRed(), 255u),
          core::min_(getGreen() + other.getGreen(), 255u),
          core::min_(getBlue() + other.getBlue(), 255u));
          }*/

      //! Interpolates the color with a float value to another color
      /** \param other: Other color
      \param d: value between 0.0f and 1.0f
      \return Interpolated color. */
      SColor getInterpolated(const SColor &other, float d) const
      {
        //			d = clamp(d, 0.f, 1.f);
        const float inv = 1.0f - d;
        return SColor((unsigned)roundf(other.getAlpha()*inv + getAlpha()*d),
          (unsigned)roundf(other.getRed()*inv + getRed()*d),
          (unsigned)roundf(other.getGreen()*inv + getGreen()*d),
          (unsigned)roundf(other.getBlue()*inv + getBlue()*d));
      }

      //! Returns interpolated color. ( quadratic )
      /** \param c1: first color to interpolate with
      \param c2: second color to interpolate with
      \param d: value between 0.0f and 1.0f. */
      /*		SColor getInterpolated_quadratic(const SColor& c1, const SColor& c2, float d) const
          {
          // this*(1-d)*(1-d) + 2 * c1 * (1-d) + c2 * d * d;
          d = clampf(d, 0.f, 1.f);
          const float inv = 1.f - d;
          const float mul0 = inv * inv;
          const float mul1 = 2.f * d * inv;
          const float mul2 = d * d;

          return SColor(
          clampf( floorf(
          getAlpha() * mul0 + c1.getAlpha() * mul1 + c2.getAlpha() * mul2 ), 0, 255 ),
          clampf( floorf(
          getRed()   * mul0 + c1.getRed()   * mul1 + c2.getRed()   * mul2 ), 0, 255 ),
          clampf( floorf(
          getGreen() * mul0 + c1.getGreen() * mul1 + c2.getGreen() * mul2 ), 0, 255 ),
          clampf( floorf(
          getBlue()  * mul0 + c1.getBlue()  * mul1 + c2.getBlue()  * mul2 ), 0, 255 ));
          }*/

      //! set the color by expecting data in the given format
      /** \param data: must point to valid memory containing color information in the given format
        \param format: tells the format in which data is available
        */
      void setData(const void *data, ECOLOR_FORMAT format)
      {
        switch (format)
        {
        case ECF_A1R5G5B5:
          color = A1R5G5B5toA8R8G8B8(*(short*)data);
          break;
        case ECF_R5G6B5:
          color = R5G6B5toA8R8G8B8(*(short*)data);
          break;
        case ECF_A8R8G8B8:
          color = *(unsigned*)data;
          break;
        case ECF_R8G8B8:
        {
          char* p = (char*)data;
          set(255, p[0], p[1], p[2]);
        }
        break;
        default:
          color = 0xffffffff;
          break;
        }
      }

      //! Write the color to data in the defined format
      /** \param data: target to write the color. Must contain sufficiently large memory to receive the number of bytes neede for format
        \param format: tells the format used to write the color into data
        */
      void getData(void *data, ECOLOR_FORMAT format)
      {
        switch (format)
        {
        case ECF_A1R5G5B5:
        {
          short * dest = (short*)data;
          *dest = video::A8R8G8B8toA1R5G5B5(color);
        }
        break;

        case ECF_R5G6B5:
        {
          short * dest = (short*)data;
          *dest = video::A8R8G8B8toR5G6B5(color);
        }
        break;

        case ECF_R8G8B8:
        {
          char* dest = (char*)data;
          dest[0] = (char)getRed();
          dest[1] = (char)getGreen();
          dest[2] = (char)getBlue();
        }
        break;

        case ECF_A8R8G8B8:
        {
          unsigned * dest = (unsigned*)data;
          *dest = color;
        }
        break;

        default:
          break;
        }
      }

      //! color in A8R8G8B8 Format
      unsigned color;
    };


    //! Class representing a color with four floats.
    /** The color values for red, green, blue
    and alpha are each stored in a 32 bit floating point variable.
    So all four values may be between 0.0f and 1.0f.
    Another, faster way to define colors is using the class SColor, which
    stores the color values in a single 32 bit integer.
    */
    class SColorf
    {
    public:
      //! Default constructor for SColorf.
      /** Sets red, green and blue to 0.0f and alpha to 1.0f. */
      SColorf() : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}

      //! Constructs a color from up to four color values: red, green, blue, and alpha.
      /** \param r: Red color component. Should be a value between
      0.0f meaning no red and 1.0f, meaning full red.
      \param g: Green color component. Should be a value between 0.0f
      meaning no green and 1.0f, meaning full green.
      \param b: Blue color component. Should be a value between 0.0f
      meaning no blue and 1.0f, meaning full blue.
      \param a: Alpha color component of the color. The alpha
      component defines how transparent a color should be. Has to be
      a value between 0.0f and 1.0f, 1.0f means not transparent
      (opaque), 0.0f means fully transparent. */
      SColorf(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

      //! Constructs a color from 32 bit Color.
      /** \param c: 32 bit color from which this SColorf class is
      constructed from. */
      SColorf(SColor c)
      {
        const float inv = 1.0f / 255.0f;
        r = c.getRed() * inv;
        g = c.getGreen() * inv;
        b = c.getBlue() * inv;
        a = c.getAlpha() * inv;
      }

      //! Converts this color to a SColor without floats.
      SColor toSColor() const
      {
        return SColor((unsigned)roundf(a*255.0f), (unsigned)roundf(r*255.0f), (unsigned)roundf(g*255.0f), (unsigned)roundf(b*255.0f));
      }

      //! Sets three color components to new values at once.
      /** \param rr: Red color component. Should be a value between 0.0f meaning
      no red (=black) and 1.0f, meaning full red.
      \param gg: Green color component. Should be a value between 0.0f meaning
      no green (=black) and 1.0f, meaning full green.
      \param bb: Blue color component. Should be a value between 0.0f meaning
      no blue (=black) and 1.0f, meaning full blue. */
      void set(float rr, float gg, float bb) { r = rr; g = gg; b = bb; }

      //! Sets all four color components to new values at once.
      /** \param aa: Alpha component. Should be a value between 0.0f meaning
      fully transparent and 1.0f, meaning opaque.
      \param rr: Red color component. Should be a value between 0.0f meaning
      no red and 1.0f, meaning full red.
      \param gg: Green color component. Should be a value between 0.0f meaning
      no green and 1.0f, meaning full green.
      \param bb: Blue color component. Should be a value between 0.0f meaning
      no blue and 1.0f, meaning full blue. */
      void set(float aa, float rr, float gg, float bb) { a = aa; r = rr; g = gg; b = bb; }

      //! Interpolates the color with a float value to another color
      /** \param other: Other color
      \param d: value between 0.0f and 1.0f
      \return Interpolated color. */
      SColorf getInterpolated(const SColorf &other, float d) const
      {
        //			d = core::clamp(d, 0.f, 1.f);
        const float inv = 1.0f - d;
        return SColorf(other.r*inv + r*d,
          other.g*inv + g*d, other.b*inv + b*d, other.a*inv + a*d);
      }

      //! Returns interpolated color. ( quadratic )
      /** \param c1: first color to interpolate with
      \param c2: second color to interpolate with
      \param d: value between 0.0f and 1.0f. */
      inline SColorf getInterpolated_quadratic(const SColorf& c1, const SColorf& c2,
        float d) const
      {
        //			d = core::clamp(d, 0.f, 1.f);
        // this*(1-d)*(1-d) + 2 * c1 * (1-d) + c2 * d * d;
        const float inv = 1.f - d;
        const float mul0 = inv * inv;
        const float mul1 = 2.f * d * inv;
        const float mul2 = d * d;

        return SColorf(r * mul0 + c1.r * mul1 + c2.r * mul2,
          g * mul0 + c1.g * mul1 + c2.g * mul2,
          b * mul0 + c1.b * mul1 + c2.b * mul2,
          a * mul0 + c1.a * mul1 + c2.a * mul2);
      }


      //! Sets a color component by index. R=0, G=1, B=2, A=3
      void setColorComponentValue(int index, float value)
      {
        switch (index)
        {
        case 0: r = value; break;
        case 1: g = value; break;
        case 2: b = value; break;
        case 3: a = value; break;
        }
      }

      //! Returns the alpha component of the color in the range 0.0 (transparent) to 1.0 (opaque)
      float getAlpha() const { return a; }

      //! Returns the red component of the color in the range 0.0 to 1.0
      float getRed() const { return r; }

      //! Returns the green component of the color in the range 0.0 to 1.0
      float getGreen() const { return g; }

      //! Returns the blue component of the color in the range 0.0 to 1.0
      float getBlue() const { return b; }

      //! red color component
      float r;

      //! green color component
      float g;

      //! blue component
      float b;

      //! alpha color component
      float a;
    };


    //! Class representing a color in HSL format
    /** The color values for hue, saturation, luminance
    are stored in 32bit floating point variables. Hue is in range [0,360],
    Luminance and Saturation are in percent [0,100]
    */
    class SColorHSL
    {
    public:
      SColorHSL(float h = 0.f, float s = 0.f, float l = 0.f)
        : Hue(h), Saturation(s), Luminance(l) {}

      void fromRGB(const SColorf &color);
      void toRGB(SColorf &color) const;

      float Hue;
      float Saturation;
      float Luminance;

    private:
      inline float toRGB1(float rm1, float rm2, float rh) const;

    };

    /*	inline void SColorHSL::fromRGB(const SColorf &color)
      {
      float maxVal = core::max_(color.getRed(), color.getGreen(), color.getBlue());
      float minVal = (float)core::min_(color.getRed(), color.getGreen(), color.getBlue());
      Luminance = (maxVal+minVal)*50;
      if (core::equals(maxVal, minVal))
      {
      Hue=0.f;
      Saturation=0.f;
      return;
      }

      const float delta = maxVal-minVal;
      if ( Luminance <= 50 )
      {
      Saturation = (delta)/(maxVal+minVal);
      }
      else
      {
      Saturation = (delta)/(2-maxVal-minVal);
      }
      Saturation *= 100;

      if (maxVal == color.getRed())
      Hue = (color.getGreen()-color.getBlue())/delta;
      else if (maxVal == color.getGreen())
      Hue = 2+((color.getBlue()-color.getRed())/delta);
      else // blue is max
      Hue = 4+((color.getRed()-color.getGreen())/delta);

      Hue *= 60.0f;
      while ( Hue < 0.f )
      Hue += 360;
      }*/


    inline void SColorHSL::toRGB(SColorf &color) const
    {
      const float l = Luminance / 100;
      if (Saturation == 0.f) // grey
      {
        color.set(l, l, l);
        return;
      }

      float rm2;

      if (Luminance <= 50)
      {
        rm2 = l + l * (Saturation / 100);
      }
      else
      {
        rm2 = l + (1 - l) * (Saturation / 100);
      }

      const float rm1 = 2.0f * l - rm2;

      const float h = Hue / 360.0f;
      color.set(toRGB1(rm1, rm2, h + 1.f / 3.f),
        toRGB1(rm1, rm2, h),
        toRGB1(rm1, rm2, h - 1.f / 3.f)
        );
    }


    // algorithm from Foley/Van-Dam
    inline float SColorHSL::toRGB1(float rm1, float rm2, float rh) const
    {
      if (rh < 0)
        rh += 1;
      if (rh > 1)
        rh -= 1;

      if (rh < 1.f / 6.f)
        rm1 = rm1 + (rm2 - rm1) * rh*6.f;
      else if (rh < 0.5f)
        rm1 = rm2;
      else if (rh < 2.f / 3.f)
        rm1 = rm1 + (rm2 - rm1) * ((2.f / 3.f) - rh)*6.f;

      return rm1;
    }

  } // end namespace video
} // end namespace irr

#endif
