#version 330

layout(location = 0) in vec2 Position;
layout(location = 1) in vec2 Texcoord;

out vec2 uv;

void main()
{
   gl_Position = vec4(Position, 1., 1.);
   uv = Texcoord;
}