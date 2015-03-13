
#version 430
uniform mat4 ModelMatrix;
uniform mat4 ViewProjectionMatrix;

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

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Tangent;

// Need Depth, gl_FragCoord.z returns meaningless values...
out float depth;
out vec4 tangent;
out vec4 p0p1;

// From VS_RenderHair_AA

void main(void) {
// Hardcoded values, need to go in cb
  float expandPixels = 0.71;

  // Calculate right and projected right vectors
  vec3 v = (g_mWorld * vec4(Position, 1.)).xyz;
  vec3 right = normalize( cross( Tangent, normalize(v - g_vEye)));
  vec2 proj_right = normalize( (g_mViewProj * vec4(right, 0)).xy );

  vec4 hairEdgePositions0, hairEdgePositions1; // 0 is negative, 1 is positive
  hairEdgePositions0 = vec4(v +  -1.0 * right * g_FiberRadius, 1.0);
  hairEdgePositions1 = vec4(v +   1.0 * right * g_FiberRadius, 1.0);
  hairEdgePositions0 = g_mViewProj * hairEdgePositions0;
  hairEdgePositions1 = g_mViewProj * hairEdgePositions1;
  hairEdgePositions0 = hairEdgePositions0 / hairEdgePositions0.w;
  hairEdgePositions1 = hairEdgePositions1 / hairEdgePositions1.w;

  float fDirIndex = (gl_VertexID % 2 == 1) ? -1.0 : 1.0;

  gl_Position = (fDirIndex==-1.0 ? hairEdgePositions0 : hairEdgePositions1) + fDirIndex * vec4(proj_right * expandPixels / g_WinSize.y, 0.0, 0.0);
  depth = gl_Position.z;
  // Wrong if g_mWorld rescale
  tangent = g_mWorld * vec4(Tangent, 0.);
  p0p1 = vec4( hairEdgePositions0.xy, hairEdgePositions1.xy );
}