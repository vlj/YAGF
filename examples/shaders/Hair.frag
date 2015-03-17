#version 430 core
layout (binding = 0) uniform atomic_uint PixelCount;
layout(r32ui) uniform restrict uimage2D PerPixelLinkedListHead;

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


//--------------------------------------------------------------------------------------
// Helper functions for packing and unpacking the stored tangent and coverage
//--------------------------------------------------------------------------------------
uint PackFloat4IntoUint(vec4 vValue)
{
  uint byte3 = uint(vValue.x * 255.) & 0xFF;
  uint byte2 = uint(vValue.y * 255.) & 0xFF;
  uint byte1 = uint(vValue.z*255) & 0xFF;
  uint byte0 = uint(vValue.w * 255) & 0xFF;
  return (byte3 << 24 ) | (byte2 << 16 ) | (byte1 << 8) | byte0;
}

vec4 UnpackUintIntoFloat4(uint uValue)
{
    return vec4( ( (uValue & 0xFF000000)>>24 ) / 255.0, ( (uValue & 0x00FF0000)>>16 ) / 255.0, ( (uValue & 0x0000FF00)>>8 ) / 255.0, ( (uValue & 0x000000FF) ) / 255.0);
}

uint PackTangentAndCoverage(vec3 tangent, float coverage)
{
    return PackFloat4IntoUint(vec4(tangent.xyz*0.5 + 0.5, coverage) );
}

vec3 GetTangent(uint packedTangent)
{
    return 2.0 * UnpackUintIntoFloat4(packedTangent).xyz - 1.0;
}

float GetCoverage(uint packedCoverage)
{
    return UnpackUintIntoFloat4(packedCoverage).w;
}


struct PerPixelListBucket
{
    float depth;
    uint TangentAndCoverage;
    uint next;
};

layout(std430, binding = 0) restrict writeonly buffer PerPixelLinkedList
{
    PerPixelListBucket PPLL[10000000];
};

//--------------------------------------------------------------------------------------
// ComputeCoverage
//
// Calculate the pixel coverage of a hair strand by computing the hair width
//--------------------------------------------------------------------------------------
float ComputeCoverage(vec2 p0, vec2 p1, vec2 pixelLoc)
{
  // p0, p1, pixelLoc are in d3d clip space (-1 to 1)x(-1 to 1)

  // Scale positions so 1.f = half pixel width
  p0 *= g_WinSize.xy;
  p1 *= g_WinSize.xy;
  pixelLoc *= g_WinSize.xy;

  float p0dist = length(p0 - pixelLoc);
  float p1dist = length(p1 - pixelLoc);
  float hairWidth = length(p0 - p1);

  // will be 1.f if pixel outside hair, 0.f if pixel inside hair
  bool outside = any( bvec2(step(hairWidth, p0dist) > 0., step(hairWidth, p1dist) > 0.));

  // if outside, set sign to -1, else set sign to 1
  float sign = outside ? -1.f : 1.f;

  // signed distance (positive if inside hair, negative if outside hair)
  float relDist = sign * clamp(min(p0dist, p1dist), 0., 1.);

  // returns coverage based on the relative distance
  // 0, if completely outside hair edge
  // 1, if completely inside hair edge
  return (relDist + 1.f) * 0.5f;
}

void StoreFragments_Hair(vec2 FragCoordXY, vec3 Tangent, float Coverage, float depth)
{
  uint pixel_id = atomicCounterIncrement(PixelCount);
  int pxid = int(pixel_id);
  ivec2 iuv = ivec2(FragCoordXY);
  uint tmp = imageAtomicExchange(PerPixelLinkedListHead, iuv, pixel_id);
  PPLL[pxid].depth = depth;
  PPLL[pxid].TangentAndCoverage = PackTangentAndCoverage(Tangent.xyz, Coverage);
  PPLL[pxid].next = tmp;
}

in float depth;
in vec4 tangent;
in vec4 p0p1;
out vec4 FragColor;

void main() {
    // Render AA Line, calculate pixel coverage
  vec4 proj_pos = vec4(2 * gl_FragCoord.xy * g_WinSize.zw - 1., 1., 1.);
  vec4 original_pos = g_mInvViewProj * proj_pos; // Not used ??

  float coverage = 1.f;
  coverage = ComputeCoverage(p0p1.xy, p0p1.zw, proj_pos.xy);

  coverage *= g_FiberAlpha * tangent.w * g_FiberRadius;
  if (coverage > g_alphaThreshold)
    StoreFragments_Hair(gl_FragCoord.xy, tangent.xyz, coverage, .5 * depth + .5);
  FragColor = vec4(1.);
}