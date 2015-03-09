// Copyright (C) 2015 Vincent Lejeune
// Basic text rendering
// Require Freetype
// Doesn't hand complex typographic layout (eg arabic, chinese)
// Pango could do that but it's unfortunatly hard to use...

#include <GL/glew.h>
#include <Core/Shaders.h>

extern "C" {
#include <ft2build.h>
#include FT_FREETYPE_H
}

class GlyphRendering : public ShaderHelperSingleton<GlyphRendering, irr::core::vector2df, irr::core::vector2df>, public TextureRead<Texture2D>
{
public:
    GlyphRendering()
    {
        const char *glyphvs =
            "#version 330\n"
            "uniform vec2 GlyphCenter;\n"
            "uniform vec2 GlyphScaling;\n"
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
        AssignUniforms("GlyphCenter", "GlyphScaling");

        AssignSamplerNames(Program, 0, "tex");
    }
};

template<int GlyphSize>
class BasicTextRender : public Singleton <BasicTextRender<GlyphSize> >
{
    FT_Library Ft;
    FT_Face Face;

    GLuint GlyphVAO;
    GLuint GlyphVBO;

    GLuint GlyphTexture[256];
    struct GlyphData
    {
        int width;
        int height;
        int bitmap_left;
        int bitmap_top;
        int advance_x;
        int advance_y;
    };

    GlyphData Glyph[128];
    GLuint Sampler;

public:
    BasicTextRender()
    {
        if (FT_Init_FreeType(&Ft))
            printf("Can't init freetype\n");
#ifdef WIN32
        if (FT_New_Face(Ft, "C:\\Windows\\Fonts\\arial.ttf", 0, &Face))
            printf("Can't init Arial\n");
#else
        if (FT_New_Face(Ft, "/usr/share/fonts/dejavu/DejaVuSansMono.ttf", 0, &Face))
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
            if (FT_Load_Glyph(Face, FT_Get_Char_Index(Face, i), FT_LOAD_TARGET_LIGHT))
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
    }

    ~BasicTextRender()
    {
        glDeleteVertexArrays(1, GlyphVAO);
        glDeleteBuffers(1, GlyphVBO);
        glDeleteTextures(128, GlyphTexture);
        glDeleteSamplers(1, Sampler);
    }

    void drawText(const char *text, size_t posX, size_t posY, size_t screenWidth, size_t screenHeight)
    {

        glBindVertexArray(GlyphVAO);
        glUseProgram(GlyphRendering::getInstance()->Program);

        irr::core::vector2df pixelSize(2. / screenWidth, 2. / screenHeight);
        irr::core::vector2df screenpos(posX, posY);
        screenpos *= pixelSize;
        screenpos -= irr::core::vector2df(1., 1.);

        for (const char *p = text; *p != '\0'; p++)
        {
            const GlyphData &g = Glyph[*p];
            irr::core::vector2df truescreenpos(screenpos);
            truescreenpos += irr::core::vector2df(g.bitmap_left, - g.bitmap_top) * pixelSize;
            GlyphRendering::getInstance()->SetTextureUnits(GlyphTexture[*p], Sampler);
            GlyphRendering::getInstance()->setUniforms(truescreenpos, irr::core::vector2df(g.width, g.height) * pixelSize);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            screenpos += irr::core::vector2df(g.advance_x >> 6, g.advance_y >> 6) * pixelSize;
        }
    }
};