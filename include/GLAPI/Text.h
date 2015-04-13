// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt


// Basic text rendering
// Require Freetype
// Doesn't hand complex typographic layout (eg arabic, chinese)
// Pango could do that but it's unfortunatly hard to use...

#include <GL/glew.h>
#include <GLAPI/Shaders.h>

extern "C" {
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H
}

class GlyphRendering : public ShaderHelperSingleton<GlyphRendering>, public TextureRead <UniformBufferResource<0>, TextureResource<GL_TEXTURE_2D, 0> >
{
public:
  GlyphRendering()
  {
    const char *glyphvs =
      "#version 330\n"
      "layout(std140) uniform GlyphBuffer {\n"
      "  vec2 GlyphCenter;\n"
      "  vec2 GlyphScaling;\n"
      "};"
      "layout(location = 0) in vec2 Position;\n"
      "layout(location = 1) in vec2 Texcoord;\n"
      "out vec2 uv;"
      "void main()\n"
      "{\n"
      "   uv = Texcoord;\n"
      "   gl_Position = vec4(1., -1., 1., 1.) * vec4(GlyphCenter + GlyphScaling * Position, 0., 1.);\n"
      "}\n";

    const char *passthrougfs =
      "#version 330\n"
      "uniform sampler2D tex;\n"
      "in vec2 uv;"
      "out vec4 FragColor;\n"
      "void main() {\n"
      "FragColor = vec4(texture(tex,uv).r, 0., 0., texture(tex,uv).r);\n"
      "}\n";

    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, glyphvs,
      GL_FRAGMENT_SHADER, passthrougfs);

    AssignSamplerNames(Program, "GlyphBuffer", "tex");
  }
};

template<int GlyphSize>
class BasicTextRender : public Singleton < BasicTextRender<GlyphSize> >
{
  FT_Library Ft;
  FT_Face Face;

  GLuint GlyphVAO;
  GLuint GlyphVBO;
  GLuint GlyphUBO;

  GLuint GlyphTexture[256];
  struct GlyphData
  {
    unsigned int width;
    unsigned int height;
    int bitmap_left;
    int bitmap_top;
    long int advance_x;
    long int advance_y;
  };

  struct GlyphBuffer
  {
    float GlyphCenter[2];
    float GlyphScaling[2];
  };

  GlyphData Glyph[128];
  GLuint Sampler;

public:
  BasicTextRender()
  {
    if (FT_Init_FreeType(&Ft))
      printf("Can't init freetype\n");
    FT_Library_SetLcdFilter(Ft, FT_LCD_FILTER_LIGHT);
#ifdef WIN32
    if (FT_New_Face(Ft, "C:\\Windows\\Fonts\\arial.ttf", 0, &Face))
      printf("Can't init Arial\n");
#else
    if (FT_New_Face(Ft, "/usr/share/fonts/truetype/freefont/FreeMono.ttf", 0, &Face))
      printf("Can't init DejaVuSansMono\n");
#endif
    FT_Set_Pixel_Sizes(Face, GlyphSize, GlyphSize);

    glGenVertexArrays(1, &GlyphVAO);
    glBindVertexArray(GlyphVAO);
    glGenBuffers(1, &GlyphVBO);
    glBindBuffer(GL_ARRAY_BUFFER, GlyphVBO);
    const float quad_vertex[] = {
      0., 0., 0., 0., // UpperLeft
      0., 1., 0., 1., // LowerLeft
      1., 0., 1., 0., // UpperRight
      1., 1., 1., 1., // LowerRight
    };
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid *)(2 * sizeof(float)));
    glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), quad_vertex, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenTextures(128, GlyphTexture);
    glGenSamplers(1, &Sampler);
    glSamplerParameteri(Sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(Sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(Sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(Sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    for (int i = 0; i < 128; i++)
    {
      if (FT_Load_Glyph(Face, FT_Get_Char_Index(Face, i), FT_LOAD_TARGET_LCD))
        printf("Could not load character %c\n", i);
      if (FT_Render_Glyph(Face->glyph, FT_RENDER_MODE_LIGHT))
        printf("Could not render character %c\n", i);

      glBindTexture(GL_TEXTURE_2D, GlyphTexture[i]);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Face->glyph->bitmap.width, Face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, Face->glyph->bitmap.buffer);
      Glyph[i] = { Face->glyph->bitmap.width, Face->glyph->bitmap.rows, Face->glyph->bitmap_left, Face->glyph->bitmap_top, Face->glyph->advance.x, Face->glyph->advance.y };
    }
    FT_Done_Face(Face);
    FT_Done_FreeType(Ft);

    glGenBuffers(1, &GlyphUBO);
  }

  ~BasicTextRender()
  {
    glDeleteVertexArrays(1, GlyphVAO);
    glDeleteBuffers(1, GlyphVBO);
    glDeleteTextures(128, GlyphTexture);
    glDeleteSamplers(1, Sampler);
    glDeleteBuffers(1, &GlyphUBO);
  }

  void drawText(const char *text, size_t posX, size_t posY, size_t screenWidth, size_t screenHeight)
  {

    glBindVertexArray(GlyphVAO);
    glUseProgram(GlyphRendering::getInstance()->Program);

    irr::core::vector2df pixelSize(2.f / screenWidth, 2.f / screenHeight);
    irr::core::vector2df screenpos((float)posX, (float)posY);
    screenpos *= pixelSize;
    screenpos -= irr::core::vector2df(1.f, 1.f);

    struct GlyphBuffer cbufdata;

    for (const char *p = text; *p != '\0'; p++)
    {
      const GlyphData &g = Glyph[*p];
      irr::core::vector2df truescreenpos(screenpos);
      truescreenpos += irr::core::vector2df((float)g.bitmap_left, -(float)g.bitmap_top) * pixelSize;
      cbufdata.GlyphCenter[0] = truescreenpos.X;
      cbufdata.GlyphCenter[1] = truescreenpos.Y;
      cbufdata.GlyphScaling[0] = (float)g.width * pixelSize.X;
      cbufdata.GlyphScaling[1] = (float)g.height * pixelSize.Y;
      glBindBuffer(GL_UNIFORM_BUFFER, GlyphUBO);
      glBufferData(GL_UNIFORM_BUFFER, sizeof(struct GlyphBuffer), &cbufdata, GL_STATIC_DRAW);
      GlyphRendering::getInstance()->SetTextureUnits(GlyphUBO, GlyphTexture[*p], Sampler);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      screenpos += irr::core::vector2df((float)(g.advance_x >> 6), (float) (g.advance_y >> 6)) * pixelSize;
    }
  }
};
