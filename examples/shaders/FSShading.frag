#version 430 core
layout(r32ui) uniform volatile restrict uimage2D PerPixelLinkedListHead;
out vec4 FragColor;

struct PerPixelListBucket
{
     float depth;
      vec3 tangent;
     uint next;
};

layout(std430, binding = 1) buffer PerPixelLinkedList
{
    PerPixelListBucket PPLL[1000000];
};

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
        vec3 ttmp = PPLL[ListBucketId].tangent;
        PPLL[ListBucketId].tangent = PPLL[NextListBucketId].tangent;
        PPLL[NextListBucketId].tangent = ttmp;
      }
      ListBucketId = NextListBucketId;
      NextListBucketId = PPLL[NextListBucketId].next;
    }
  }
}

void main() {
  ivec2 iuv = ivec2(gl_FragCoord.xy);
  uint ListBucketHead = imageLoad(PerPixelLinkedListHead, iuv).x;
  if (ListBucketHead == 0) discard;
  BubbleSort(ListBucketHead);
  uint ListBucketId = ListBucketHead;
  vec4 result = vec4(0., 0., 0., 1.);
  int tmp = 1;
  while (ListBucketId != 0) {
    float NdotL = max(0., dot(vec3(1., 1., 0.), PPLL[ListBucketId].tangent));
    result.xyz += sqrt(1. - (pow(NdotL, 2.)));
    result *= .1;
    ListBucketId = PPLL[ListBucketId].next;
  }
  FragColor = vec4(1.) / tmp;
};