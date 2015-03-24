// If you change the value below, you must change it in TressFXAsset.h as well.
#define THREAD_GROUP_SIZE 64
#define MAX_VERTS_PER_STRAND 16

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
  unsigned int* globalStrandIndex,
  unsigned int* localStrandIndex,
  unsigned int* globalVertexIndex,
  unsigned int* localVertexIndex,
  unsigned int* numVerticesInTheStrand,
  unsigned int* indexForSharedMem,
  unsigned int* strandType)
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

void CalcIndicesInStrandLevelMaster(
  unsigned int local_id,
  unsigned int group_id,
  int g_NumOfStrandsPerThreadGroup,
  int g_NumFollowHairsPerOneGuideHair,
  __global const int* g_HairStrandType,
  unsigned int *globalStrandIndex,
  unsigned int *numVerticesInTheStrand,
  unsigned int *globalRootVertexIndex,
  unsigned int *strandType)
{
  *globalStrandIndex = THREAD_GROUP_SIZE * group_id + local_id;
  *globalStrandIndex *= (g_NumFollowHairsPerOneGuideHair + 1);
  *numVerticesInTheStrand = (THREAD_GROUP_SIZE / g_NumOfStrandsPerThreadGroup);
  *strandType = g_HairStrandType[*globalStrandIndex];
  *globalRootVertexIndex = *globalStrandIndex * *numVerticesInTheStrand;
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
  outputPos.xyz = curPosition.xyz + (1.f - dampingCoeff)*(curPosition.xyz - oldPosition.xyz) + force.xyz * timeStep * timeStep;

  return outputPos;
}


float4 MultQuaternionAndQuaternion(float4 qA, float4 qB)
{
  float4 q;

  q.w = qA.w * qB.w - qA.x * qB.x - qA.y * qB.y - qA.z * qB.z;
  q.x = qA.w * qB.x + qA.x * qB.w + qA.y * qB.z - qA.z * qB.y;
  q.y = qA.w * qB.y + qA.y * qB.w + qA.z * qB.x - qA.x * qB.z;
  q.z = qA.w * qB.z + qA.z * qB.w + qA.x * qB.y - qA.y * qB.x;

  return q;
}

float3 MultQuaternionAndVector(float4 q, float3 v)
{
  float3 uv, uuv;
  float3 qvec = (float3)(q.x, q.y, q.z);
  uv = cross(qvec, v);
  uuv = cross(qvec, uv);
  uv *= (2.0f * q.w);
  uuv *= 2.0f;

  return v + uv + uuv;
}

float4 MakeQuaternion(float angle_radian, float3 axis)
{
  // create quaternion using angle and rotation axis
  float4 quaternion;
  float halfAngle = 0.5f * angle_radian;
  float sinHalf = sin(halfAngle);

  quaternion.w = cos(halfAngle);
  quaternion.xyz = sinHalf * axis.xyz;

  return quaternion;
}

float4 InverseQuaternion(float4 q)
{
  float lengthSqr = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;

  if (lengthSqr < 0.001f)
    return (float4)(0.f, 0.f, 0.f, 1.0f);

  q.x = -q.x / lengthSqr;
  q.y = -q.y / lengthSqr;
  q.z = -q.z / lengthSqr;
  q.w = q.w / lengthSqr;

  return q;
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
  __constant struct SimulationConstants* params,
  unsigned int vertexCount)
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
  if (globalVertexIndex < vertexCount)
  {
    g_HairVertexPositionsPrev[globalVertexIndex] = currentPos;
    g_HairVertexPositions[globalVertexIndex] = tmpPos;
  }
}


//--------------------------------------------------------------------------------------
//
//  LocalShapeConstraintsWithIteration
//
//  Compute shader to maintain the local shape constraints. This is the same as
//  the LocalShapeConstraints shader, except the iterations are done on the GPU
//  instead of multiple dispatch calls on the CPU, for better performance
//
//--------------------------------------------------------------------------------------

__kernel void LocalShapeConstraintsWithIteration(
  __global float4 *g_HairVertexPositions,
  __global const int *g_HairStrandType,
  __global const float4 *g_GlobalRotations,
  __global const float4 *g_HairRefVecsInLocalFrame,
  __constant struct SimulationConstants* params,
  unsigned int vertexCount)
{
  uint globalStrandIndex, numVerticesInTheStrand, globalRootVertexIndex, strandType;
  CalcIndicesInStrandLevelMaster(get_local_id(0), get_group_id(0),
    params->NumOfStrandsPerThreadGroup, params->NumFollowHairsPerOneGuideHair,
    g_HairStrandType,
    &globalStrandIndex,
    &numVerticesInTheStrand,
    &globalRootVertexIndex,
    &strandType);

  // stiffness for local shape constraints
  float stiffnessForLocalShapeMatching = 0.4f;

  if ( strandType == 2)
    stiffnessForLocalShapeMatching = params->StiffnessForLocalShapeMatching2;
  else if ( strandType == 3 )
    stiffnessForLocalShapeMatching = params->StiffnessForLocalShapeMatching3;
  else if ( strandType == 1 )
    stiffnessForLocalShapeMatching = params->StiffnessForLocalShapeMatching1;
  else if ( strandType == 0 )
    stiffnessForLocalShapeMatching = params->StiffnessForLocalShapeMatching0;

  //1.0 for stiffness makes things unstable sometimes.
  stiffnessForLocalShapeMatching = 0.5f * min(stiffnessForLocalShapeMatching, 0.95f);

  //------------------------------
  // Copy strand data into registers, for faster iteration
  //------------------------------
  unsigned int globalVertexIndex = 0;
  float4 sharedStrandPos[MAX_VERTS_PER_STRAND];
  for (unsigned int localVertexIndex = 0; localVertexIndex < numVerticesInTheStrand; localVertexIndex++)
  {
    globalVertexIndex = globalRootVertexIndex + localVertexIndex;
    sharedStrandPos[localVertexIndex] = g_HairVertexPositions[globalVertexIndex];
  }

  //--------------------------------------------
  // Local shape constraint for bending/twisting
  //--------------------------------------------
  for (unsigned int iterations = 0; iterations < params->NumLocalShapeMatchingIterations; iterations++)
  {
    float4 pos = sharedStrandPos[1];
    float4 rotGlobal = g_GlobalRotations[globalRootVertexIndex];

    for (unsigned int localVertexIndex = 1; localVertexIndex < numVerticesInTheStrand - 1; localVertexIndex++)
    {
      globalVertexIndex = globalRootVertexIndex + localVertexIndex;
      float4 pos_plus_one = sharedStrandPos[localVertexIndex + 1];

      //--------------------------------
      // Update position i and i_plus_1
      //--------------------------------
      float4 rotGlobalWorld = MultQuaternionAndQuaternion(params->ModelRotateForHead, rotGlobal);

      float3 orgPos_i_plus_1_InLocalFrame_i = g_HairRefVecsInLocalFrame[globalVertexIndex + 1].xyz;
      float3 orgPos_i_plus_1_InGlobalFrame = MultQuaternionAndVector(rotGlobalWorld, orgPos_i_plus_1_InLocalFrame_i) + pos.xyz;

      float3 del = stiffnessForLocalShapeMatching * (orgPos_i_plus_1_InGlobalFrame - pos_plus_one.xyz).xyz;

      if (IsMovable(pos))
        pos.xyz -= del.xyz;

      if (IsMovable(pos_plus_one))
        pos_plus_one.xyz += del.xyz;

      //---------------------------
      // Update local/global frames
      //---------------------------
      float4 invRotGlobalWorld = InverseQuaternion(rotGlobalWorld);
      float3 vec = normalize(pos_plus_one.xyz - pos.xyz);

      float3 x_i_plus_1_frame_i = normalize(MultQuaternionAndVector(invRotGlobalWorld, vec));
      float3 e = (float3)(1.f, 0.f, 0.f);
      float3 rotAxis = cross(e, x_i_plus_1_frame_i);

      if ( length(rotAxis) > 0.001f )
      {
        float angle_radian = acos(dot(e, x_i_plus_1_frame_i));
        rotAxis = normalize(rotAxis);

        float4 localRot = MakeQuaternion(angle_radian, rotAxis);
        rotGlobal = MultQuaternionAndQuaternion(rotGlobal, localRot);
      }

      sharedStrandPos[localVertexIndex].xyz = pos.xyz;
      sharedStrandPos[localVertexIndex + 1].xyz = pos_plus_one.xyz;

      pos = pos_plus_one;
    }
  }

  for (unsigned int localVertexIndex = 0; localVertexIndex < numVerticesInTheStrand; localVertexIndex++)
  {
    globalVertexIndex = globalRootVertexIndex + localVertexIndex;
    if (globalVertexIndex < vertexCount)
      g_HairVertexPositions[globalVertexIndex] = sharedStrandPos[localVertexIndex];
  }

  return;
}