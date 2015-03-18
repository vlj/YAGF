#version 430 core
uniform sampler2D HairShadowMap;
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

#define SM_EPSILON 0.01
#define KERNEL_SIZE 5
#define PI 3.14

//--------------------------------------------------------------------------------------
// ComputeShadow
//
// Computes the shadow using a simplified deep shadow map technique for the hair and
// PCF for scene objects. It uses multiple taps to filter over a (KERNEL_SIZE x KERNEL_SIZE)
// kernel for high quality results.
//--------------------------------------------------------------------------------------
float ComputeShadow(vec3 worldPos, float alpha)
{
  vec4 projPosLight = g_mViewProjLight * vec4(worldPos,1);
  projPosLight.xy /= projPosLight.w;
  vec2 texSM = .5 * projPosLight.xy + .5;
  float depth = projPosLight.z / projPosLight.w;
  float epsilon = depth * SM_EPSILON;
  float depth_fragment = projPosLight.w;

  // for shadow casted by scene objs, use PCF shadow
  float total_weight = 0;
  float amountLight_hair = 0;

  total_weight = 0;
  for (int dx = (1 - KERNEL_SIZE) / 2; dx <= KERNEL_SIZE / 2; dx++)
  {
    for (int dy = (1-KERNEL_SIZE) / 2; dy <= KERNEL_SIZE / 2; dy++)
    {
      float size = 2.4;
      float sigma = (KERNEL_SIZE / 2.0) / size; // standard deviation, when kernel/2 > 3*sigma, it's close to zero, here we use 1.5 instead
      float exp = -1* (dx * dx + dy * dy)/ (2* sigma * sigma);
      float weight = 1 / (2 * PI * sigma * sigma) * pow(2.7, exp);

      // shadow casted by hair: simplified deep shadow map
      float depthSMHair = 2. * texture(HairShadowMap, texSM + vec2(dx, dy) / 640.).x - 1.; //z/w

      float depth_smPoint = g_fNearLight / (1 - depthSMHair * (g_fFarLight - g_fNearLight) / g_fFarLight);

      float depth_range = max(0, depth_fragment-depth_smPoint);
      float numFibers =  depth_range / (g_FiberSpacing * g_FiberRadius);

      // if occluded by hair, there is at least one fiber
      numFibers += (depth_range > .00001) ? 1. : 0.;
      amountLight_hair += pow(abs(1 - alpha), numFibers) * weight;

      total_weight += weight;
    }
  }
  amountLight_hair /= total_weight;

  float amountLight_scene = 1.;//g_txSMScene.SampleCmpLevelZero(g_samShadow, texSM, depth-epsilon);

  return (amountLight_hair * amountLight_scene);
}

//--------------------------------------------------------------------------------------
// ComputeSimpleShadow
//
// Computes the shadow using a simplified deep shadow map technique for the hair and
// PCF for scene objects. This function only uses one sample, so it is faster but
// not as good quality as ComputeShadow
//--------------------------------------------------------------------------------------
float ComputeSimpleShadow(vec3 worldPos, float alpha)
{
  vec4 projPosLight = g_mViewProjLight * vec4(worldPos, 1);
  projPosLight.xy /= projPosLight.w;

  vec2 texSM = .5 * projPosLight.xy + .5;
  float depth = projPosLight.z / projPosLight.w;
  float epsilon = depth * SM_EPSILON;
  float depth_fragment = projPosLight.w;

  // shadow casted by scene
  float amountLight_scene = 1.;//g_txSMScene.SampleCmpLevelZero(g_samShadow, texSM, depth-epsilon); // TODO

  // shadow casted by hair: simplified deep shadow map
  float depthSMHair = 2. * texture(HairShadowMap, texSM).x - 1.; //z/w

  float depth_smPoint = g_fNearLight / (1. - depthSMHair * (g_fFarLight - g_fNearLight) / g_fFarLight);

  float depth_range = max(0, depth_fragment - depth_smPoint);
  float numFibers =  depth_range / (g_FiberSpacing * g_FiberRadius);

  // if occluded by hair, there is at least one fiber
  numFibers += (depth_range > .00001) ? 1. : 0.;
  float amountLight_hair = pow(abs(1 - alpha), numFibers);

  return amountLight_hair;
}

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

   return vColor * amountLight;
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
    return vColor * amountLight;
}

#define K_BUFFER 8

struct FragmentElement {
  float depth;
  uint TangentAndCoverage;
};

void main() {
  ivec2 iuv = ivec2(gl_FragCoord.xy);
  uint ListBucketHead = imageLoad(PerPixelLinkedListHead, iuv).x;
  if (ListBucketHead == 0) discard;

  FragmentElement kbuf[K_BUFFER];
  // Load first K_BUFFER element in temp array
  int kbuf_size;
  for (kbuf_size = 0; kbuf_size < K_BUFFER; kbuf_size++)
  {
    kbuf[kbuf_size].depth = PPLL[ListBucketHead].depth;
    kbuf[kbuf_size].TangentAndCoverage = PPLL[ListBucketHead].TangentAndCoverage;
    ListBucketHead = PPLL[ListBucketHead].next;
    if (ListBucketHead == 0)
      break;
  }

  uint ListBucketId = ListBucketHead;
  vec4 result = vec4(0., 0., 0., 1.);

  while (ListBucketId != 0) {
    float max_depth = 0.;
    uint max_idx = 0;
    for (int i = 0; i < kbuf_size; i++)
    {
      float d = kbuf[kbuf_size].depth;
      max_depth = max(max_depth, d);
      max_idx = (max_depth == d) ? i : max_idx;
    }

    if (PPLL[ListBucketId].depth < max_depth)
    {
      float tmp = PPLL[ListBucketId].depth;
      PPLL[ListBucketId].depth = kbuf[max_idx].depth;
      kbuf[max_idx].depth = tmp;
      uint tmpu = PPLL[ListBucketId].TangentAndCoverage;
      PPLL[ListBucketId].TangentAndCoverage = kbuf[max_idx].TangentAndCoverage;
      kbuf[max_idx].TangentAndCoverage = tmpu;
    }

    float d = PPLL[ListBucketId].depth;
    vec3 ndcPos = 2. * vec3(gl_FragCoord.xy * g_WinSize.zw, d) - 1.;
    vec4 clipPos;
    clipPos.w = g_mViewProj[3][2] / (ndcPos.z - (g_mViewProj[2][2] / g_mViewProj[2][3]));
    clipPos.xyz = ndcPos.xyz * clipPos.w;
    vec4 Pos = g_mInvViewProj * clipPos;
    Pos /= Pos.w;
    vec3 Tangent = GetTangent(PPLL[ListBucketId].TangentAndCoverage);
    float FragmentAlpha = GetCoverage(PPLL[ListBucketId].TangentAndCoverage);
    float amountOfLight = ComputeShadow(Pos.xyz, g_HairShadowAlpha);
    vec3 FragmentColor = SimpleHairShading(Pos.xyz, Tangent, vec4(0.), amountOfLight);
    result.xyz = result.xyz * (1. - FragmentAlpha) + FragmentAlpha * FragmentColor;
    result.w *= (1. - FragmentAlpha);

    ListBucketId = PPLL[ListBucketId].next;
  }

  uint reverse = 0;
  bool isSorted = false;
  while (!isSorted) {
    isSorted = true;
    for (int i = 0; i < kbuf_size - 1; i++)
    {
      if (kbuf[i].depth < kbuf[i + 1].depth) {
        isSorted = false;
        float tmp = kbuf[i].depth;
        kbuf[i].depth = kbuf[i + 1].depth;
        kbuf[i + 1].depth = tmp;
        uint ttmp = kbuf[i].TangentAndCoverage;
        kbuf[i].TangentAndCoverage = kbuf[i + 1].TangentAndCoverage;
        kbuf[i + 1].TangentAndCoverage = ttmp;
        reverse++;
      }
    }
  }

// #define DEBUG
#ifdef DEBUG
  if (reverse < 1)
    FragColor = vec4(1., 0., 0., 1.);
  else if (reverse < 4)
    FragColor = vec4(0., 1., 0., 1.);
  else
    FragColor = vec4(0., 0., 1., 1.);
  return;
#endif

  for (int i = 0; i < kbuf_size; i++)
  {
    float d = kbuf[i].depth;
    vec3 ndcPos = 2. * vec3(gl_FragCoord.xy * g_WinSize.zw, d) - 1.;
    vec4 clipPos;
    clipPos.w = g_mViewProj[3][2] / (ndcPos.z - (g_mViewProj[2][2] / g_mViewProj[2][3]));
    clipPos.xyz = ndcPos.xyz * clipPos.w;
    vec4 Pos = g_mInvViewProj * clipPos;
    Pos /= Pos.w;

    vec3 Tangent = GetTangent(kbuf[i].TangentAndCoverage);
    float FragmentAlpha = GetCoverage(kbuf[i].TangentAndCoverage);
    float amountOfLight = ComputeShadow(Pos.xyz, g_HairShadowAlpha);
    vec3 FragmentColor = ComputeHairShading(Pos.xyz, Tangent, vec4(0.), amountOfLight);

    result.xyz = result.xyz * (1. - FragmentAlpha) + FragmentAlpha * FragmentColor;
    result.w *= (1. - FragmentAlpha);
  }
  FragColor = result;
};