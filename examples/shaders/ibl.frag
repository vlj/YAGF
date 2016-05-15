#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout(set = 0, binding = 4) uniform texture2D ctex;
layout(set = 0, binding = 5) uniform texture2D ntex;
layout(set = 0, binding = 14) uniform texture2D roughness_metalness;
layout(set = 0, binding = 6) uniform texture2D dtex;


layout(set = 2, binding = 11) uniform textureCube probe;
layout(set = 2, binding = 12) uniform texture2D dfg;

layout(set = 3, binding = 3) uniform sampler s;
layout(set = 3, binding = 13) uniform sampler bilinear_s;

layout(set = 1, binding = 7, std140) uniform VIEWDATA
{
	mat4 ViewMatrix;
	mat4 InverseViewMatrix;
	mat4 ProjectionMatrix;
	mat4 InverseProjectionMatrix;
};


layout(set = 2, binding = 10, std140) uniform IBLDATA
{
  float bL00;
  float bL1m1;
  float bL10;
  float bL11;
  float bL2m2;
  float bL2m1;
  float bL20;
  float bL21;
  float bL22;

  float gL00;
  float gL1m1;
  float gL10;
  float gL11;
  float gL2m2;
  float gL2m1;
  float gL20;
  float gL21;
  float gL22;

  float rL00;
  float rL1m1;
  float rL10;
  float rL11;
  float rL2m2;
  float rL2m1;
  float rL20;
  float rL21;
  float rL22;
};

vec3 DecodeNormal(vec2 n)
{
  float z = dot(n, n) * 2. - 1.;
  vec2 xy = normalize(n) * sqrt(1. - z * z);
  return vec3(xy,z);
}

vec4 getPosFromUVDepth(vec3 uvDepth, mat4 InverseProjectionMatrix)
{
    vec4 pos = 2.0 * vec4(uvDepth, 1.0) - 1.0f;
    pos.xy *= vec2(InverseProjectionMatrix[0][0], InverseProjectionMatrix[1][1]);
    pos.zw = vec2(pos.z * InverseProjectionMatrix[2][2] + pos.w, pos.z * InverseProjectionMatrix[2][3] + pos.w);
    pos /= pos.w;
    return pos;
}

// From "An Efficient Representation for Irradiance Environment Maps" article
// See http://graphics.stanford.edu/papers/envmap/
// Coefficients are calculated in IBL.cpp

mat4 getMatrix(float L00, float L1m1, float L10, float L11, float L2m2, float L2m1, float L20, float L21, float L22)
{
  float c1 = 0.429043, c2 = 0.511664, c3 = 0.743125, c4 = 0.886227, c5 = 0.247708;

  return mat4(
    c1 * L22, c1 * L2m2, c1 * L21, c2 * L11,
    c1 * L2m2, - c1 * L22, c1 * L2m1, c2 * L1m1,
    c1 * L21, c1 * L2m1, c3 * L20, c2 * L10,
    c2 * L11, c2 * L1m1, c2 * L10, c4 * L00 - c5 * L20
  );
}

vec3 DiffuseIBL(vec3 normal, vec3 V, float roughness, vec3 color)
{
  // Convert normal in wobLd space (where SH coordinates were computed)
  vec4 extendednormal = transpose(ViewMatrix) * vec4(normal, 0.);
  extendednormal.w = 1.;

  mat4 rmat = getMatrix(rL00, rL1m1, rL10, rL11, rL2m2, rL2m1, rL20, rL21, rL22);
  mat4 gmat = getMatrix(gL00, gL1m1, gL10, gL11, gL2m2, gL2m1, gL20, gL21, gL22);
  mat4 bmat = getMatrix(bL00, bL1m1, bL10, bL11, bL2m2, bL2m1, bL20, bL21, bL22);

  float r = dot(extendednormal, rmat * extendednormal);
  float g = dot(extendednormal, gmat * extendednormal);
  float b = dot(extendednormal, bmat * extendednormal);

  float NdotV = clamp(dot(V, normal), 0., 1.);

  return max(vec3(r, g, b), vec3(0.)) * texture(sampler2D(dfg, bilinear_s), vec2(1. - roughness, NdotV)).b * color / 3.14;
}

vec3 SpecularIBL(vec3 normal, vec3 V, float roughness, vec3 F0)
{
  // Note : We could refine the sample direction
  vec3 sampleDirection = reflect(-V, normal);
  sampleDirection = (InverseViewMatrix * vec4(sampleDirection, 0.)).xyz;
   // Assume 8 level of lod (ie 256x256 texture)

  float lodval = 7. * roughness;
  vec3 LD = max(textureLod(samplerCube(probe, s), sampleDirection, lodval).rgb, vec3(0.));

  float NdotV = clamp(dot(V, normal), 0., 1.);
  vec2 DFG = texture(sampler2D(dfg, bilinear_s), vec2(1. - roughness, NdotV)).rg;

  return LD * (F0 * DFG.x + DFG.y);
}

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 FragColor;

void main(void)
{
    vec3 normal = normalize(DecodeNormal(2. * texture(sampler2D(ntex, bilinear_s), uv).xy - 1.));
    vec4 color = texture(sampler2D(ctex, bilinear_s), uv);
    float z = texture(sampler2D(dtex, bilinear_s), uv).x;
    float roughness = texture(sampler2D(roughness_metalness, bilinear_s), uv).x;
    float Metalness = texture(sampler2D(roughness_metalness, bilinear_s), uv).y;

    vec2 inverteduv = vec2(uv.x, 1. - uv.y);
    vec4 xpos = getPosFromUVDepth(vec3(uv, z), InverseProjectionMatrix);
    vec3 eyedir = -normalize(xpos.xyz);

    vec3 Dielectric = DiffuseIBL(normal, eyedir, roughness, color.rgb) + SpecularIBL(normal, eyedir, roughness, vec3(.04));
    vec3 Metal = SpecularIBL(normal, eyedir, roughness, color.rgb);

    FragColor = vec4(mix(Dielectric, Metal, Metalness), color.a);
}