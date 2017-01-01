#version 330
uniform sampler2D tex;

in vec2 uv;
out vec4 FragColor;

vec3 getCIEYxy(vec3 rgbColor);
vec3 getRGBFromCIEXxy(vec3 YxyColor);

void main()
{
  vec4 col = texture(tex, uv);

  //vec3 Yxy = getCIEYxy(col.rgb);
  col.rgb *= 1.5;//getRGBFromCIEXxy(vec3(3.14 * Yxy.x, Yxy.y, Yxy.z));

  vec4 perChannel = (col * (6.9 * col + .6)) / (col * (5.2 * col + 2.5) + 0.06);
  FragColor = pow(perChannel, vec4(2.2));
}

