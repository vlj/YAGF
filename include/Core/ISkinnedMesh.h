// Copyright (C) 2002-2012 Nikolaus Gebhardt
// Copyright (C) 2015 Vincent Lejeune
// Contains code from the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __I_SKINNED_MESH_H_INCLUDED__
#define __I_SKINNED_MESH_H_INCLUDED__

//#include <SceneNodes/IBoneSceneNode.h>
//#include <Core/IAnimatedMesh.h>
//#include "SSkinMeshBuffer.h"
#include <Core/IMeshBuffer.h>
#include <Maths/quaternion.h>
#include <Maths/vector3d.h>
#include <Maths/matrix4.h>
#include <string>
#include <vector>

namespace irr
{
  namespace scene
  {

    enum E_INTERPOLATION_MODE
    {
      // constant does use the current key-values without interpolation
      EIM_CONSTANT = 0,

      // linear interpolation
      EIM_LINEAR,

      //! count of all available interpolation modes
      EIM_COUNT
    };


    //! Interface for using some special functions of Skinned meshes
    class ISkinnedMesh
    {
    public:

      //! Gets joint count.
      /** \return Amount of joints in the skeletal animated mesh. */
      virtual unsigned getJointCount() const = 0;

      //! Gets the name of a joint.
      /** \param number: Zero based index of joint. The last joint
      has the number getJointCount()-1;
      \return Name of joint and null if an error happened. */
      virtual const char* getJointName(unsigned number) const = 0;

      //! Gets a joint number from its name
      /** \param name: Name of the joint.
      \return Number of the joint or -1 if not found. */
      virtual int getJointNumber(const char* name) const = 0;

      //! Use animation from another mesh
      /** The animation is linked (not copied) based on joint names
      so make sure they are unique.
      \return True if all joints in this mesh were
      matched up (empty names will not be matched, and it's case
      sensitive). Unmatched joints will not be animated. */
      virtual bool useAnimationFrom(const ISkinnedMesh *mesh) = 0;

      //! Update Normals when Animating
      /** \param on If false don't animate, which is faster.
      Else update normals, which allows for proper lighting of
      animated meshes. */
      virtual void updateNormalsWhenAnimating(bool on) = 0;

      //! Sets Interpolation Mode
      virtual void setInterpolationMode(E_INTERPOLATION_MODE mode) = 0;

      //! Animates this mesh's joints based on frame input
      virtual void animateMesh(float frame, float blend) = 0;

      //! Preforms a software skin on this mesh based of joint positions
      virtual void skinMesh(float strength = 1.f) = 0;

      //! converts the vertex type of all meshbuffers to tangents.
      /** E.g. used for bump mapping. */
      virtual void convertMeshToTangents() = 0;

      //! Allows to enable hardware skinning.
      /* This feature is not implementated in Irrlicht yet */
      virtual bool setHardwareSkinning(bool on) = 0;

      //! A vertex weight
      struct SWeight
      {
        //! Index of the mesh buffer
        unsigned short buffer_id; //I doubt 32bits is needed

                       //! Index of the vertex
        unsigned short vertex_id; //Store global ID here

                       //! Weight Strength/Percentage (0-1)
        float strength;

      private:
        //! Internal members used by CSkinnedMesh
        friend class CSkinnedMesh;
        bool *Moved;
        core::vector3df StaticPos;
        core::vector3df StaticNormal;
      };


      //! Animation keyframe which describes a new position
      struct SPositionKey
      {
        float frame;
        core::vector3df position;
      };

      //! Animation keyframe which describes a new scale
      struct SScaleKey
      {
        float frame;
        core::vector3df scale;
      };

      //! Animation keyframe which describes a new rotation
      struct SRotationKey
      {
        float frame;
        core::quaternion rotation;
      };

      //! Joints
      struct SJoint
      {
        SJoint() : UseAnimationFrom(0), GlobalSkinningSpace(false),
          positionHint(-1), scaleHint(-1), rotationHint(-1)
        {
        }

        //! The name of this joint
        std::string Name;

        //! Local matrix of this joint
        core::matrix4 LocalMatrix;

        //! List of child joints
        std::vector<SJoint*> Children;

        //! List of attached meshes
        std::vector<unsigned> AttachedMeshes;

        //! Animation keys causing translation change
        std::vector<SPositionKey> PositionKeys;

        //! Animation keys causing scale change
        std::vector<SScaleKey> ScaleKeys;

        //! Animation keys causing rotation change
        std::vector<SRotationKey> RotationKeys;

        //! Skin weights
        std::vector<SWeight> Weights;

        //! Unnecessary for loaders, will be overwritten on finalize
        core::matrix4 GlobalMatrix;
        core::matrix4 GlobalAnimatedMatrix;
        core::matrix4 LocalAnimatedMatrix;
        core::vector3df Animatedposition;
        core::vector3df Animatedscale;
        core::quaternion Animatedrotation;

        core::matrix4 GlobalInversedMatrix; //the x format pre-calculates this

      private:
        //! Internal members used by CSkinnedMesh
        friend class CSkinnedMesh;

        SJoint *UseAnimationFrom;
        bool GlobalSkinningSpace;

        int positionHint;
        int scaleHint;
        int rotationHint;
      };


      //Interface for the mesh loaders (finalize should lock these functions, and they should have some prefix like loader_

      //these functions will use the needed arrays, set values, etc to help the loaders

      //! exposed for loaders: to add mesh buffers
      virtual std::vector<irr::scene::IMeshBuffer<irr::video::S3DVertex2TCoords>*>& getMeshBuffers() = 0;

      //! exposed for loaders: joints list
      virtual std::vector<SJoint*>& getAllJoints() = 0;

      //! exposed for loaders: joints list
      virtual std::vector<SJoint*>& getAllJoints() const = 0;

      //! loaders should call this after populating the mesh
      virtual void finalize() = 0;

      //! Adds a new meshbuffer to the mesh, access it as last one
      virtual irr::scene::IMeshBuffer<irr::video::S3DVertex2TCoords>* addMeshBuffer() = 0;

      //! Adds a new joint to the mesh, access it as last one
      virtual SJoint* addJoint(SJoint *parent = 0) = 0;

      //! Adds a new weight to the mesh, access it as last one
      virtual SWeight* addWeight(SJoint *joint) = 0;

      //! Adds a new position key to the mesh, access it as last one
      virtual SPositionKey* addPositionKey(SJoint *joint) = 0;
      //! Adds a new scale key to the mesh, access it as last one
      virtual SScaleKey* addScaleKey(SJoint *joint) = 0;
      //! Adds a new rotation key to the mesh, access it as last one
      virtual SRotationKey* addRotationKey(SJoint *joint) = 0;

      //! Check if the mesh is non-animated
      virtual bool isStatic() = 0;
    };

  } // end namespace scene
} // end namespace irr

#endif

