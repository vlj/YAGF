#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput ctex;
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput ntex;
layout(input_attachment_index = 3, set = 0, binding = 2) uniform subpassInput dtex;

/*
layout(std140) uniform VIEWDATA
{
  mat4 ViewMatrix;
  mat4 InverseViewMatrix;
  mat4 InverseProjectionMatrix;
};

layout(std140) uniform LIGHTDATA
{
  vec3 sun_direction;
  float sun_angle;
  vec3 sun_col;
};*/

vec3 DecodeNormal(vec2 n)
{
  float z = dot(n, n) * 2. - 1.;
  vec2 xy = normalize(n) * sqrt(1. - z * z);
  return vec3(xy,z);
}
/*
// Burley model from Frostbite going pbr paper
vec3 DiffuseBRDF(vec3 normal, vec3 eyedir, vec3 lightdir, vec3 color, float roughness)
{
    float biaised_roughness = 0.05 + 0.95 * roughness;
    // Half Light View direction
    vec3 H = normalize(eyedir + lightdir);
    float LdotH = clamp(dot(lightdir, H), 0., 1.);
    float NdotL = clamp(dot(lightdir, normal), 0., 1.);
    float NdotV = clamp(dot(lightdir, eyedir), 0., 1.);

    float Fd90 = 0.5 + 2 * LdotH * LdotH * biaised_roughness * biaised_roughness;
    float SchlickFresnelL = (1. + (Fd90 - 1.) * (1. - pow(NdotL, 5.)));
    float SchlickFresnelV = (1. + (Fd90 - 1.) * (1. - pow(NdotV, 5.)));
    return color * SchlickFresnelL * SchlickFresnelV / 3.14;
}

// Fresnel Schlick approximation
vec3 Fresnel(vec3 viewdir, vec3 halfdir, vec3 basecolor)
{
    return clamp(basecolor + (1. - basecolor) * pow(1. - clamp(dot(viewdir, halfdir), 0., 1.), 5.), vec3(0.), vec3(1.));
}


// Schlick geometry term
float G1(vec3 V, vec3 normal, float k)
{
    float NdotV = clamp(dot(V, normal), 0., 1.);
    return 1. / (NdotV * (1. - k) + k);
}

// Smith model
// We factor the (n.v) (n.l) factor in the denominator
float ReducedGeometric(vec3 lightdir, vec3 viewdir, vec3 normal, float roughness)
{
    float k = (roughness + 1.) * (roughness + 1.) / 8.;
    return G1(lightdir, normal, k) * G1(viewdir, normal, k);
}

// GGX
float Distribution(float roughness, vec3 normal, vec3 halfdir)
{
    float alpha = roughness * roughness * roughness * roughness;
    float NdotH = clamp(dot(normal, halfdir), 0., 1.);
    float normalisationFactor = 1. / 3.14;
    float denominator = NdotH * NdotH * (alpha - 1.) + 1.;
    return normalisationFactor * alpha / (denominator * denominator);
}

vec3 SpecularBRDF(vec3 normal, vec3 eyedir, vec3 lightdir, vec3 color, float roughness)
{
    // Half Light View direction
    vec3 H = normalize(eyedir + lightdir);
    float biaised_roughness = 0.05 + 0.95 * roughness;

    // Microfacet model
    return Fresnel(eyedir, H, color) * ReducedGeometric(lightdir, eyedir, normal, biaised_roughness) * Distribution(biaised_roughness, normal, H) / 4.;
}

vec4 getPosFromUVDepth(vec3 uvDepth, mat4 InverseProjectionMatrix)
{
    vec4 pos = 2.0 * vec4(uvDepth, 1.0) - 1.0f;
    pos.xy *= vec2(InverseProjectionMatrix[0][0], InverseProjectionMatrix[1][1]);
    pos.zw = vec2(pos.z * InverseProjectionMatrix[2][2] + pos.w, pos.z * InverseProjectionMatrix[2][3] + pos.w);
    pos /= pos.w;
    return pos;
}

// Sun Most Representative Point (used for MRP area lighting method)
// From "Frostbite going PBR" paper
vec3 SunMRP(vec3 normal, vec3 eyedir)
{
    vec3 local_sundir = normalize((transpose(InverseViewMatrix) * vec4(sun_direction, 0.)).xyz);
    vec3 R = reflect(-eyedir, normal);
    float angularRadius = 3.14 * sun_angle / 180.;
    vec3 D = local_sundir;
    float d = cos(angularRadius);
    float r = sin(angularRadius);
    float DdotR = dot(D, R);
    vec3 S = R - DdotR * D;
    return (DdotR < d) ? normalize(d * D + normalize (S) * r) : R;
}*/

in vec2 uv;
layout(location = 0) out vec4 FragColor;

void main() {
    vec3 color = subpassLoad(ctex).xyz;
    vec3 norm = normalize(DecodeNormal(2. * subpassLoad(ntex).xy - 1.));

    FragColor = subpassLoad(dtex);

/*
    float z = texture(dtex, uv).x;
    vec4 xpos = getPosFromUVDepth(vec3(uv, z), InverseProjectionMatrix);
    float roughness = texture(ntex, uv).z;
    vec3 eyedir = -normalize(xpos.xyz);

    vec3 Lightdir = SunMRP(norm, eyedir);
    float NdotL = clamp(dot(norm, Lightdir), 0., 1.);

    float metalness = texture(ntex, uv).a;

    vec3 Dielectric = DiffuseBRDF(norm, eyedir, Lightdir, color, roughness) + SpecularBRDF(norm, eyedir, Lightdir, vec3(.04), roughness);
    vec3 Metal = SpecularBRDF(norm, eyedir, Lightdir, color, roughness);
    FragColor = vec4(NdotL * sun_col * mix(Dielectric, Metal, metalness), 0.);*/
}
