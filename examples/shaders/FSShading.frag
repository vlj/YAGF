#version 430 core
layout(r32ui) uniform volatile restrict uimage2D PerPixelLinkedListHead;

layout(std140, binding = 0) uniform Constants
{
  mat4 g_mWorld;
  mat4 g_mViewProj;
  mat4 g_mInvViewProj;
  mat4 g_mViewProjLight;

  vec3 g_vEye;
  float g_fvFov;

  vec4 g_AmbientLightColor;
  vec4 g_PointLightColor;
  vec4 g_PointLightPos;
  vec4 g_MatBaseColor;
  vec4 g_MatKValue; // Ka, Kd, Ks, Ex

  float g_FiberAlpha;
  float g_HairShadowAlpha;
  float g_bExpandPixels;
  float g_FiberRadius;

  vec4 g_WinSize; // screen size

  float g_FiberSpacing; // average spacing between fibers
  float g_bThinTip;
  float g_fNearLight;
  float g_fFarLight;

  int g_iTechSM;
  int g_bUseCoverage;
  int g_iStrandCopies; // strand copies that the transparency shader will produce
  int g_iMaxFragments;

  float g_alphaThreshold;
  float g_fHairKs2; // for second highlight
  float g_fHairEx2; // for second highlight

  mat4 g_mInvViewProjViewport;
};

out vec4 FragColor;

struct PerPixelListBucket
{
     float depth;
     uint TangentAndCoverage;
     uint next;
};

layout(std430, binding = 0) buffer PerPixelLinkedList
{
    PerPixelListBucket PPLL[1000000];
};

//--------------------------------------------------------------------------------------
// Helper functions for packing and unpacking the stored tangent and coverage
//--------------------------------------------------------------------------------------
vec4 UnpackUintIntoFloat4(uint uValue)
{
    return vec4( ( (uValue & 0xFF000000)>>24 ) / 255.0, ( (uValue & 0x00FF0000)>>16 ) / 255.0, ( (uValue & 0x0000FF00)>>8 ) / 255.0, ( (uValue & 0x000000FF) ) / 255.0);
}

vec3 GetTangent(uint packedTangent)
{
    return 2.0 * UnpackUintIntoFloat4(packedTangent).xyz - 1.0;
}

float GetCoverage(uint packedCoverage)
{
    return UnpackUintIntoFloat4(packedCoverage).w;
}

void BubbleSort(uint ListBucketHead) {
  bool isSorted = false;
  while (!isSorted) {
    isSorted = true;
    uint ListBucketId = ListBucketHead;
    uint NextListBucketId = PPLL[ListBucketId].next;
    while (NextListBucketId != 0) {
    if (PPLL[ListBucketId].depth < PPLL[NextListBucketId].depth) {
        isSorted = false;
        float tmp = PPLL[ListBucketId].depth;
        PPLL[ListBucketId].depth = PPLL[NextListBucketId].depth;
        PPLL[NextListBucketId].depth = tmp;
        uint ttmp = PPLL[ListBucketId].TangentAndCoverage;
        PPLL[ListBucketId].TangentAndCoverage = PPLL[NextListBucketId].TangentAndCoverage;
        PPLL[NextListBucketId].TangentAndCoverage = ttmp;
      }
      ListBucketId = NextListBucketId;
      NextListBucketId = PPLL[NextListBucketId].next;
    }
  }
}

#define PI 3.14

vec3 ComputeHairShading(vec3 iPos, vec3 iTangent, vec4 iTex, float amountLight)
{
    vec3 baseColor = g_MatBaseColor.xyz;
    float rand_value = 1;

//    if(abs(iTex.x) + abs(iTex.y) >1e-5) // if texcoord is available, use texture map
//        rand_value = g_txNoise.SampleLevel(g_samLinearWrap, iTex.xy, 0).x;

    // define baseColor and Ka Kd Ks coefficient for hair
    float Ka = g_MatKValue.x, Kd = g_MatKValue.y,
          Ks1 = g_MatKValue.z, Ex1 = g_MatKValue.w,
          Ks2 = g_fHairKs2, Ex2 = g_fHairEx2;

    vec3 lightPos = g_PointLightPos.xyz;
    vec3 vLightDir = normalize(lightPos - iPos.xyz);
    vec3 vEyeDir = normalize(g_vEye.xyz - iPos.xyz);
    vec3 tangent = normalize(iTangent);

    // in Kajiya's model: diffuse component: sin(t, l)
    float cosTL = (dot(tangent, vLightDir));
    float sinTL = sqrt(1 - cosTL*cosTL);
    float diffuse = sinTL; // here sinTL is apparently larger than 0

    float alpha = (rand_value * 10) * PI/180; // tiled angle (5-10 dgree)

    // in Kajiya's model: specular component: cos(t, rl) * cos(t, e) + sin(t, rl)sin(t, e)
    float cosTRL = -cosTL;
    float sinTRL = sinTL;
    float cosTE = (dot(tangent, vEyeDir));
    float sinTE = sqrt(1- cosTE*cosTE);

    // primary highlight: reflected direction shift towards root (2 * Alpha)
    float cosTRL_root = cosTRL * cos(2 * alpha) - sinTRL * sin(2 * alpha);
    float sinTRL_root = sqrt(1 - cosTRL_root * cosTRL_root);
    float specular_root = max(0, cosTRL_root * cosTE + sinTRL_root * sinTE);

    // secondary highlight: reflected direction shifted toward tip (3*Alpha)
    float cosTRL_tip = cosTRL*cos(-3*alpha) - sinTRL*sin(-3*alpha);
    float sinTRL_tip = sqrt(1 - cosTRL_tip * cosTRL_tip);
    float specular_tip = max(0, cosTRL_tip * cosTE + sinTRL_tip * sinTE);

    vec3 vColor = Ka * g_AmbientLightColor.xyz * baseColor + // ambient
                    amountLight * g_PointLightColor.xyz * (
                    Kd * diffuse * baseColor + // diffuse
                    Ks1 * pow(specular_root, Ex1)  + // primary hightlight r
                    Ks2 * pow(specular_tip, Ex2) * baseColor); // secondary highlight rtr

   return vColor;
}

vec3 SimpleHairShading(vec3 iPos, vec3 iTangent, vec4 iTex, float amountLight)
{
  vec3 baseColor = g_MatBaseColor.xyz;
  float Kd = g_MatKValue.y;

#ifdef SUPERSIMPLESHADING
  vec3 vColor = amountLight * Kd * baseColor;
#else
  // define baseColor and Ka Kd Ks coefficient for hair
  float Ka = g_MatKValue.x;
  float Ks1 = g_MatKValue.z;
  float Ex1 = g_MatKValue.w;
  float Ks2 = g_fHairKs2;
  float Ex2 = g_fHairEx2;

  vec3 lightPos = g_PointLightPos.xyz;
  vec3 vLightDir = normalize(lightPos - iPos.xyz);
  vec3 tangent = normalize(iTangent);

  // in Kajiya's model: diffuse component: sin(t, l)
  float cosTL = (dot(tangent, vLightDir));
  float sinTL = sqrt(1 - cosTL*cosTL);
  float diffuse = sinTL; // here sinTL is apparently larger than 0

  vec3 vColor = Ka * g_AmbientLightColor.xyz * baseColor +                          // ambient
                  amountLight * g_PointLightColor.xyz * (Kd * diffuse * baseColor); // diffuse
#endif
    return vColor;
}

void main() {
  ivec2 iuv = ivec2(gl_FragCoord.xy);
  uint ListBucketHead = imageLoad(PerPixelLinkedListHead, iuv).x;
  if (ListBucketHead == 0) discard;
  BubbleSort(ListBucketHead);
  uint ListBucketId = ListBucketHead;
  vec4 result = vec4(0., 0., 0., 1.);
  while (ListBucketId != 0) {
    vec3 Tangent = GetTangent(PPLL[ListBucketId].TangentAndCoverage);
    float cos = dot(vec3(0., -1., 0.), Tangent);
    result.xyz += sqrt(1. - cos * cos) * g_MatBaseColor.xyz;
    result *= .5;
    ListBucketId = PPLL[ListBucketId].next;
  }
  FragColor = result;
};