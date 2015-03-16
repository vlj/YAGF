

// If you change the value below, you must change it in TressFXAsset.h as well.
#define THREAD_GROUP_SIZE 64

struct SimulationConstants
{
  float ModelTransformForHead[16];
  float4 ModelRotateForHead;

  float4 Wind;
  float4 Wind1;
  float4 Wind2;
  float4 Wind3;

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



//--------------------------------------------------------------------------------------
//
//  Helper Functions for the main simulation kernel
//
//--------------------------------------------------------------------------------------
bool IsMovable(float4 particle)
{
  return particle.w > 0;
}

void CalcIndicesInVertexLevelMaster(
  unsigned int local_id, unsigned int group_id,
  int g_NumOfStrandsPerThreadGroup,
  int g_NumFollowHairsPerOneGuideHair,
  __global const int* g_HairStrandType,
  uint* globalStrandIndex,
  uint* localStrandIndex,
  uint* globalVertexIndex,
  uint* localVertexIndex,
  uint* numVerticesInTheStrand,
  uint* indexForSharedMem,
  uint* strandType)
{
  *indexForSharedMem = local_id;
  *numVerticesInTheStrand = (THREAD_GROUP_SIZE / g_NumOfStrandsPerThreadGroup);

  *localStrandIndex = local_id % g_NumOfStrandsPerThreadGroup;
  *globalStrandIndex = group_id * g_NumOfStrandsPerThreadGroup + *localStrandIndex;
  *globalStrandIndex *= (g_NumFollowHairsPerOneGuideHair + 1);
  *localVertexIndex = (local_id - *localStrandIndex) / g_NumOfStrandsPerThreadGroup;

  *strandType = g_HairStrandType[*globalStrandIndex];
  *globalVertexIndex = *globalStrandIndex * *numVerticesInTheStrand + *localVertexIndex;
}

//--------------------------------------------------------------------------------------
//
//  Integrate
//
//  Uses Verlet integration to calculate the new position for the current time step
//
//--------------------------------------------------------------------------------------
float4 Integrate(
  float4 curPosition,
  float4 oldPosition,
  float4 initialPos,
  float4 force,
  uint globalVertexIndex,
  uint localVertexIndex,
  uint numVerticesInTheStrand,
  float timeStep,
  float g_GravityMagnitude,
  float dampingCoeff)
{
  float4 outputPos = curPosition;

  force.xyz += g_GravityMagnitude * (float3)(0.f, -1.f, 0.f);
  outputPos.xyz = curPosition.xyz + (1.0 - dampingCoeff)*(curPosition.xyz - oldPosition.xyz) + force.xyz * timeStep * timeStep;

  return outputPos;
}

float4 mulMatrixVector(__constant float *Mat, float4 vector)
{
  float4 result;
  result.x = Mat[0] * vector.x + Mat[4] * vector.y + Mat[8] * vector.z + Mat[12] * vector.w;
  result.y = Mat[1] * vector.x + Mat[5] * vector.y + Mat[9] * vector.z + Mat[13] * vector.w;
  result.z = Mat[2] * vector.x + Mat[6] * vector.y + Mat[10] * vector.z + Mat[14] * vector.w;
  result.w = Mat[3] * vector.x + Mat[7] * vector.y + Mat[11] * vector.z + Mat[15] * vector.w;

  return result;
}

//--------------------------------------------------------------------------------------
//
//  IntegrationAndGlobalShapeConstraints
//
//  Compute shader to simulate the gravitational force with integration and to maintain the
//  global shape constraints.
//
// One thread computes one vertex.
//
//--------------------------------------------------------------------------------------
__kernel 
void IntegrationAndGlobalShapeConstraints(
  __global float4 *g_HairVertexPositions,
  __global float4 *g_HairVertexPositionsPrev,
  __global const int *g_HairStrandType,
  __global const float4 *g_InitialHairPositions,
  __constant struct SimulationConstants* params)
{
  unsigned int globalStrandIndex, localStrandIndex, globalVertexIndex, localVertexIndex, numVerticesInTheStrand, indexForSharedMem, strandType;
  CalcIndicesInVertexLevelMaster(get_local_id(0), get_group_id(0),
    params->NumOfStrandsPerThreadGroup, params->NumFollowHairsPerOneGuideHair,
    g_HairStrandType,
    &globalStrandIndex,
    &localStrandIndex,
    &globalVertexIndex,
    &localVertexIndex,
    &numVerticesInTheStrand,
    &indexForSharedMem,
    &strandType);

  float4 currentPos = (float4)(0.f, 0.f, 0.f, 0.f); // position when this step starts. In other words, a position from the last step.
  float4 initialPos = (float4)(0.f, 0.f, 0.f, 0.f); // rest position

  float4 tmpPos;

  // Copy data into shared memory
  initialPos = g_InitialHairPositions[globalVertexIndex];
  initialPos.xyz = mulMatrixVector(params->ModelTransformForHead, (float4) (initialPos.xyz, 1)).xyz;
  if (params->bWarp)
    currentPos = initialPos;
  else
    currentPos = tmpPos = g_HairVertexPositions[globalVertexIndex];

  // Integrate
  float dampingCoeff = 0.03f;

  if ( strandType == 0 )
    dampingCoeff = params->Damping0;
  else if ( strandType == 1 )
    dampingCoeff = params->Damping1;
  else if ( strandType == 2 )
    dampingCoeff = params->Damping2;
  else if ( strandType == 3 )
    dampingCoeff = params->Damping3;

  float4 oldPos;
  if (params->bWarp)
    oldPos = currentPos;
  else
    oldPos = g_HairVertexPositionsPrev[globalVertexIndex];
  float4 force = (float4) (0.f, 0.f, 0.f, 0.f);

  if ( IsMovable(currentPos) )
    tmpPos = Integrate(currentPos, oldPos, initialPos, force, globalVertexIndex, localVertexIndex, numVerticesInTheStrand, params->timeStep, params->GravityMagnitude, dampingCoeff);
  else
    tmpPos = initialPos;

  // Global Shape Constraints
  float stiffnessForGlobalShapeMatching = 0;
  float globalShapeMatchingEffectiveRange = 0;

  if ( strandType == 0 )
  {
    stiffnessForGlobalShapeMatching = params->StiffnessForGlobalShapeMatching0;
    globalShapeMatchingEffectiveRange = params->GlobalShapeMatchingEffectiveRange0;
  }
  else if ( strandType == 1 )
  {
    stiffnessForGlobalShapeMatching = params->StiffnessForGlobalShapeMatching1;
    globalShapeMatchingEffectiveRange = params->GlobalShapeMatchingEffectiveRange1;
  }
  else if ( strandType == 2 )
  {
    stiffnessForGlobalShapeMatching = params->StiffnessForGlobalShapeMatching2;
    globalShapeMatchingEffectiveRange = params->GlobalShapeMatchingEffectiveRange2;
  }
  else if ( strandType == 3 )
  {
    stiffnessForGlobalShapeMatching = params->StiffnessForGlobalShapeMatching3;
    globalShapeMatchingEffectiveRange = params->GlobalShapeMatchingEffectiveRange3;
  }

  if ( stiffnessForGlobalShapeMatching > 0 && globalShapeMatchingEffectiveRange != 0.)
  {
    if (IsMovable(tmpPos))
    {
      if ((float)localVertexIndex < globalShapeMatchingEffectiveRange * (float)numVerticesInTheStrand)
      {
        float factor = stiffnessForGlobalShapeMatching;
        float3 del = factor * (initialPos - tmpPos).xyz;
        tmpPos.xyz += del;
      }
    }
  }

  // update global position buffers
  g_HairVertexPositionsPrev[globalVertexIndex] = currentPos;
  g_HairVertexPositions[globalVertexIndex] = tmpPos;
}