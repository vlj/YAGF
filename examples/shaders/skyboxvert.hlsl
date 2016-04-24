struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};


PS_INPUT main(float2 pos : POSITION, float2 texc : TEXCOORD0)
{
  PS_INPUT result;
  result.pos = float4(pos.x, -pos.y, 1., 1.);
  result.uv = float2(texc.x, texc.y);
  return result;
}