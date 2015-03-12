#version 430 core
layout (binding = 0) uniform atomic_uint PixelCount;
layout(r32ui) uniform volatile restrict uimage2D PerPixelLinkedListHead;
uniform vec4 color;
in float depth;
in vec3 tangent;
out vec4 FragColor;
struct PerPixelListBucket
{
    float depth;
    uint TangentAndCoverage;
    uint next;
};
layout(std430, binding = 1) buffer PerPixelLinkedList
{
    PerPixelListBucket PPLL[10000000];
};
void main() {
  uint pixel_id = atomicCounterIncrement(PixelCount);
  int pxid = int(pixel_id);
  ivec2 iuv = ivec2(gl_FragCoord.xy);
  uint tmp = imageAtomicExchange(PerPixelLinkedListHead, iuv, pixel_id);
  PPLL[pxid].depth = depth;
//  PPLL[pxid].TangentAndCoverage = tangent;
  PPLL[pxid].next = tmp;
  FragColor = vec4(0.);
}