cbuffer SceneMatrixes : register(b0, space7)
{
	float4x4 ViewMatrix;
	float4x4 InverseViewMatrix;
	float4x4 ProjectionMatrix;
	float4x4 InverseProjectionMatrix;
}

cbuffer ObjectMatrixes : register(b0, space0)
{
	float4x4 ModelMatrix;
	float4x4 InverseModelMatrix;
}

struct VS_INPUT
{
  float3 pos : POSITION;
  float2 texc : TEXCOORD0;
  float3 normal : NORMAL;
/*  int index0 : TEXCOORD1;
  float weight0 : TEXCOORD2;
  int index1 : TEXCOORD3;
  float weight1 : TEXCOORD4;
  int index2 : TEXCOORD5;
  float weight2 : TEXCOORD6;
  int index3 : TEXCOORD7;
  float weight3 : TEXCOORD8;*/
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
  float3 normal : TEXCOORD1;
};

PS_INPUT main(VS_INPUT In)
{
  PS_INPUT result;
  float4 IdlePosition = float4(In.pos.x, In.pos.y, In.pos.z, 1.);
/*  float4 SkinnedPosition = float4(0., 0., 0., 0.);

  float4 SingleBoneInfluencedPosition;
  if (In.index0 >= 0)
  {
    SingleBoneInfluencedPosition = mul(JointTransform[In.index0], IdlePosition);
    SingleBoneInfluencedPosition /= SingleBoneInfluencedPosition.w;
  }
  else
    SingleBoneInfluencedPosition = IdlePosition;
  SkinnedPosition += In.weight0 * SingleBoneInfluencedPosition;

  if (In.index1 >= 0)
  {
    SingleBoneInfluencedPosition = mul(JointTransform[In.index1], IdlePosition);
    SingleBoneInfluencedPosition /= SingleBoneInfluencedPosition.w;
  }
  else
    SingleBoneInfluencedPosition = IdlePosition;
  SkinnedPosition += In.weight1 * SingleBoneInfluencedPosition;

  if (In.index2 >= 0)
  {
    SingleBoneInfluencedPosition = mul(JointTransform[In.index2], IdlePosition);
    SingleBoneInfluencedPosition /= SingleBoneInfluencedPosition.w;
  }
  else
    SingleBoneInfluencedPosition = IdlePosition;
  SkinnedPosition += In.weight2 * SingleBoneInfluencedPosition;

  if (In.index3 >= 0)
  {
    SingleBoneInfluencedPosition = mul(JointTransform[In.index3], IdlePosition);
    SingleBoneInfluencedPosition /= SingleBoneInfluencedPosition.w;
  }
  else
    SingleBoneInfluencedPosition = IdlePosition;
  SkinnedPosition += In.weight3 * SingleBoneInfluencedPosition;*/

  float4 position = mul(ModelMatrix, IdlePosition);
  result.pos = mul(mul(ProjectionMatrix, ViewMatrix), position);
  result.uv = In.texc;
  result.normal = mul(transpose(mul(InverseModelMatrix, InverseViewMatrix)), float4(In.normal, 0.)).xyz;
  return result;
}