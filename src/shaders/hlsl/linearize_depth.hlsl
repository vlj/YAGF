
cbuffer CONSTANT_BUF : register(b0, space0)
{
  float zn;
  float zf;
};

Texture2D Depth_Texture : register(t0, space1);

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};


float4 main(PS_INPUT In) : SV_TARGET
{
	int3 coord = int3(In.uv * 1024, 0);
	float d = Depth_Texture.Load(coord);
	float c0 = zn * zf;
	float c1 = zn - zf;
	float c2 = zf;
	return c0 / (d * c1 + c2);
}