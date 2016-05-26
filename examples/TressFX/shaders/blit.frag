#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout(set = 0, binding = 0) uniform texture2D tex;
layout(set = 0, binding = 1) uniform sampler s;

layout(location = 0) out vec4 FragColor;

void main(void)
{
    FragColor = texture(sampler2D(tex, s), gl_FragCoord.xy / 1024.).x > 0.999 ? vec4(1.) : vec4(0.);
}

