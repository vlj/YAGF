#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
                         "DescriptorTable(CBV(b0))"

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
PS_INPUT main(float4 pos : POSITION, float2 texc : TEXCOORD0)
{
  PS_INPUT result;
  float4 position = mul(ModelMatrix, float4(pos.x, pos.y, pos.z, 1.));
  result.pos = mul(ViewProjectionMatrix, position);
  result.uv = texc;
  return result;
}