#version 330
uniform samplerCube skytexture;

layout(std140) uniform Matrixes
{
  mat4 InvView;
  mat4 InvProj;
};

in vec2 uv;
out vec4 FragColor;

void main(void)
{
    vec3 eyedir = vec3(uv, 1.);
    eyedir = 2.0 * eyedir - 1.0;
    vec4 tmp = (InvProj * vec4(eyedir, 1.));
    tmp /= tmp.w;
    eyedir = (InvView * vec4(tmp.xyz, 0.)).xyz;
    vec4 color = texture(skytexture, eyedir);
    FragColor = vec4(color.xyz, 1.);
}

