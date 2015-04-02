#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
                         "DescriptorTable(CBV(b0), SRV(t0))," \
                         "DescriptorTable(Sampler(s0))"


Texture2D Tex : register(t0);
sampler TexSampler : register(s0);

cbuffer CONSTANT_BUF : register(b0)
{
	float4 color;
}

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

[RootSignature(MyRS1)]
float4 main(PS_INPUT In) : SV_TARGET
{
	return Tex.Sample(TexSampler, In.uv);
}