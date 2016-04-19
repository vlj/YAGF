#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 Position;
layout(location = 1) in vec2 Texcoord;

out vec2 uv;

void main()
{
   gl_Position = vec4(Position, 0., 1.);
   uv = Texcoord;
}