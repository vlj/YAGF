#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 Position;

void main()
{
   gl_Position = vec4(Position, 0., 1.);
}