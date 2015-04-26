uniform sampler2D ntex;
uniform sampler2D dtex;
uniform sampler2D ctex;
uniform sampler2D emittex;

out vec4 FragColor;

vec3 DecodeNormal(vec2 n);
vec4 getPosFromUVDepth(vec3 uvDepth, mat4 InverseProjectionMatrix);

// From "An Efficient Representation for Irradiance Environment Maps" article
// See http://graphics.stanford.edu/papers/envmap/
// Coefficients are calculated in IBL.cpp

uniform samplerCube probe;
uniform sampler2D dfg;

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

    return max(vec3(r, g, b), vec3(0.)) * texture(dfg, vec2(NdotV, roughness)).b * color / 3.14;
}

uniform samplerCube probe;
uniform sampler2D dfg;

vec3 SpecularIBL(vec3 normal, vec3 V, float roughness, vec3 F0)
{
    vec3 sampleDirection = reflect(-V, normal);
    sampleDirection = (InverseViewMatrix * vec4(sampleDirection, 0.)).xyz;
     // Assume 8 level of lod (ie 256x256 texture)

    float lodval = 7. * roughness;
    vec3 LD = max(textureLod(probe, sampleDirection, lodval).rgb, vec3(0.));

    float NdotV = clamp(dot(V, normal), 0., 1.);
    vec2 DFG = texture(dfg, vec2(NdotV, roughness)).rg;

    return LD * (F0 * DFG.x + DFG.y);
}

void main(void)
{
    vec2 uv = gl_FragCoord.xy / screen;
    vec3 normal = normalize(DecodeNormal(2. * texture(ntex, uv).xy - 1.));
    vec3 color = texture(ctex, uv).rgb;

    vec3 emitcolor = pow(color.rgb, vec3(3.)) * 15 * texture(emittex, uv).r;

    float z = texture(dtex, uv).x;
    vec4 xpos = getPosFromUVDepth(vec3(uv, z), InverseProjectionMatrix);
    vec3 eyedir = -normalize(xpos.xyz);
    float specval = texture(ntex, uv).z;

    vec3 Dielectric = DiffuseIBL(normal, eyedir, specval, color) + SpecularIBL(normal, eyedir, specval, vec3(.04));
    vec3 Metal = SpecularIBL(normal, eyedir, specval, color);
    float Metalness = texture(ntex, uv).a;

    FragColor = vec4(.2 * mix(Dielectric, Metal, Metalness) + emitcolor, texture(ctex, uv).a);
}