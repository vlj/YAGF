#include <Maths/vector3d.h>
#include <vector>

// From TressFx SDK
struct SimulationConstants
{
  float ModelTransformForHead[16];
  float ModelRotateForHead[4];

  float Wind[4];
  float Wind1[4];
  float Wind2[4];
  float Wind3[4];

  int NumLengthConstraintIterations;
  int bCollision;

  float GravityMagnitude;
  float timeStep;

  float Damping0;
  float StiffnessForLocalShapeMatching0;
  float StiffnessForGlobalShapeMatching0;
  float GlobalShapeMatchingEffectiveRange0;

  float Damping1;
  float StiffnessForLocalShapeMatching1;
  float StiffnessForGlobalShapeMatching1;
  float GlobalShapeMatchingEffectiveRange1;

  float Damping2;
  float StiffnessForLocalShapeMatching2;
  float StiffnessForGlobalShapeMatching2;
  float GlobalShapeMatchingEffectiveRange2;

  float Damping3;
  float StiffnessForLocalShapeMatching3;
  float StiffnessForGlobalShapeMatching3;
  float GlobalShapeMatchingEffectiveRange3;

  unsigned int NumOfStrandsPerThreadGroup;
  unsigned int NumFollowHairsPerOneGuideHair;

  int bWarp;
  int NumLocalShapeMatchingIterations;
};

struct Float4
{
  float X;
  float Y;
  float Z;
  float W;
};

struct StrandVertex
{
  irr::core::vector3df position;
  irr::core::vector3df tangent;
  Float4 texcoord;
};

class BSphere
{
public:
  irr::core::vector3df center;
  float radius;
  BSphere()
  {
    center = irr::core::vector3df(0, 0, 0);
    radius = 0;
  }
};

struct TressFXAsset
{
  int* m_pHairStrandType;
  Float4* m_pRefVectors;
  Float4* m_pGlobalRotations;
  Float4* m_pLocalRotations;
  Float4* m_pVertices;
  Float4* m_pTangents;
  Float4* m_pFollowRootOffset;
  StrandVertex* m_pTriangleVertices;
  float* m_pThicknessCoeffs;
  std::vector<int> m_LineIndices;
  std::vector<int> m_Triangleindices;
  float* m_pRestLengths;
  BSphere m_bSphere;
  int m_NumTotalHairStrands;
  int m_NumTotalHairVertices;
  int m_MaxNumOfVerticesInStrand;
  int m_NumGuideHairStrands;
  int m_NumGuideHairVertices;
  int m_NumFollowHairsPerOneGuideHair;
};

TressFXAsset loadTress(const char* filename);
void cleanTFXAssetContent(const TressFXAsset &tfxassets);
void initCommon(const TressFXAsset& tfxassets);
void cleanCommon();
void draw(float density);