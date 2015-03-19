
#version 430

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


layout(std430, binding = 2) restrict readonly buffer HairPos
{
  vec4 g_HairVertexPositions[1000000];
};

layout(std430, binding = 6) restrict readonly buffer HairTangent
{
  vec4 g_HairVertexTangents[1000000];
};

layout(std430, binding = 9) restrict readonly buffer HairThickness
{
  float g_HairThicknessCoeffs[1000000];
};

// Need Depth, gl_FragCoord.z returns meaningless values...
out float depth;
out vec4 tangent;
out vec4 p0p1;

// From VS_RenderHair_AA

void main(void) {
// Hardcoded values, need to go in cb
  float expandPixels = 0.71;
  vec3 v = g_HairVertexPositions[gl_VertexID / 2].xyz;
  vec3 Tangent = g_HairVertexTangents[gl_VertexID / 2].xyz;
  float ratio = g_HairThicknessCoeffs[gl_VertexID / 2];

  // Calculate right and projected right vectors
  vec3 right = normalize( cross( Tangent, normalize(v - g_vEye)));
  vec2 proj_right = normalize( (g_mViewProj * vec4(right, 0)).xy );

  vec4 hairEdgePositions0, hairEdgePositions1; // 0 is negative, 1 is positive
  hairEdgePositions0 = vec4(v - 1.0 * right * ratio * g_FiberRadius, 1.0);
  hairEdgePositions1 = vec4(v + 1.0 * right * ratio * g_FiberRadius, 1.0);
  hairEdgePositions0 = g_mViewProj * hairEdgePositions0;
  hairEdgePositions1 = g_mViewProj * hairEdgePositions1;
  hairEdgePositions0 = hairEdgePositions0 / hairEdgePositions0.w;
  hairEdgePositions1 = hairEdgePositions1 / hairEdgePositions1.w;

  float fDirIndex = (gl_VertexID % 2 == 1) ? -1.0 : 1.0;

  gl_Position = (fDirIndex==-1.0 ? hairEdgePositions0 : hairEdgePositions1) + fDirIndex * vec4(proj_right * expandPixels / g_WinSize.y, 0.0, 0.0);
  depth = gl_Position.z;
  tangent = vec4(Tangent, ratio);
  p0p1 = vec4( hairEdgePositions0.xy, hairEdgePositions1.xy );
}