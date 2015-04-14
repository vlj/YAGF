#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
                         "DescriptorTable(CBV(b0))," \
                         "DescriptorTable(SRV(t0)), " \
                         "DescriptorTable(sampler(s0))"

cbuffer CONSTANT_BUF : register(b0)
{
  float4x4 ModelMatrix;
  float4x4 ViewProjectionMatrix;
  float4x4 ProjectionMatrix;
  float4 color;
  float zn;
  float zf;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

[RootSignature(MyRS1)]
float4 main(PS_INPUT In) : SV_TARGET
{
  return color;
}