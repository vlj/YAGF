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
  namespace video
  {
    //! Abstracted and easy to use fixed function/programmable pipeline material modes.
    enum E_MATERIAL_TYPE
    {
      //! Standard solid material.
      /** Only first texture is used, which is supposed to be the
      diffuse material. */
      EMT_SOLID = 0,

      //! Solid material with 2 texture layers.
      /** The second is blended onto the first using the alpha value
      of the vertex colors. This material is currently not implemented in OpenGL.
      */
      EMT_SOLID_2_LAYER,

      //! Material type with standard lightmap technique
      /** There should be 2 textures: The first texture layer is a
      diffuse map, the second is a light map. Dynamic light is
      ignored. */
      EMT_LIGHTMAP,

      //! Material type with lightmap technique like EMT_LIGHTMAP.
      /** But lightmap and diffuse texture are added instead of modulated. */
      EMT_LIGHTMAP_ADD,

      //! Material type with standard lightmap technique
      /** There should be 2 textures: The first texture layer is a
      diffuse map, the second is a light map. Dynamic light is
      ignored. The texture colors are effectively multiplied by 2
      for brightening. Like known in DirectX as D3DTOP_MODULATE2X. */
      EMT_LIGHTMAP_M2,

      //! Material type with standard lightmap technique
      /** There should be 2 textures: The first texture layer is a
      diffuse map, the second is a light map. Dynamic light is
      ignored. The texture colors are effectively multiplyied by 4
      for brightening. Like known in DirectX as D3DTOP_MODULATE4X. */
      EMT_LIGHTMAP_M4,

      //! Like EMT_LIGHTMAP, but also supports dynamic lighting.
      EMT_LIGHTMAP_LIGHTING,

      //! Like EMT_LIGHTMAP_M2, but also supports dynamic lighting.
      EMT_LIGHTMAP_LIGHTING_M2,

      //! Like EMT_LIGHTMAP_4, but also supports dynamic lighting.
      EMT_LIGHTMAP_LIGHTING_M4,

      //! Detail mapped material.
      /** The first texture is diffuse color map, the second is added
      to this and usually displayed with a bigger scale value so that
      it adds more detail. The detail map is added to the diffuse map
      using ADD_SIGNED, so that it is possible to add and substract
      color from the diffuse map. For example a value of
      (127,127,127) will not change the appearance of the diffuse map
      at all. Often used for terrain rendering. */
      EMT_DETAIL_MAP,

      //! Look like a reflection of the environment around it.
      /** To make this possible, a texture called 'sphere map' is
      used, which must be set as the first texture. */
      EMT_SPHERE_MAP,

      //! A reflecting material with an optional non reflecting texture layer.
      /** The reflection map should be set as first texture. */
      EMT_REFLECTION_2_LAYER,

      //! A transparent material.
      /** Only the first texture is used. The new color is calculated
      by simply adding the source color and the dest color. This
      means if for example a billboard using a texture with black
      background and a red circle on it is drawn with this material,
      the result is that only the red circle will be drawn a little
      bit transparent, and everything which was black is 100%
      transparent and not visible. This material type is useful for
      particle effects. */
      EMT_TRANSPARENT_ADD_COLOR,

      //! Makes the material transparent based on the texture alpha channel.
      /** The final color is blended together from the destination
      color and the texture color, using the alpha channel value as
      blend factor. Only first texture is used. If you are using
      this material with small textures, it is a good idea to load
      the texture in 32 bit mode
      (video::IVideoDriver::setTextureCreationFlag()). Also, an alpha
      ref is used, which can be manipulated using
      SMaterial::MaterialTypeParam. This value controls how sharp the
      edges become when going from a transparent to a solid spot on
      the texture. */
      EMT_TRANSPARENT_ALPHA_CHANNEL,

      //! Makes the material transparent based on the texture alpha channel.
      /** If the alpha channel value is greater than 127, a
      pixel is written to the target, otherwise not. This
      material does not use alpha blending and is a lot faster
      than EMT_TRANSPARENT_ALPHA_CHANNEL. It is ideal for drawing
      stuff like leafes of plants, because the borders are not
      blurry but sharp. Only first texture is used. If you are
      using this material with small textures and 3d object, it
      is a good idea to load the texture in 32 bit mode
      (video::IVideoDriver::setTextureCreationFlag()). */
      EMT_TRANSPARENT_ALPHA_CHANNEL_REF,

      //! Makes the material transparent based on the vertex alpha value.
      EMT_TRANSPARENT_VERTEX_ALPHA,

      //! A transparent reflecting material with an optional additional non reflecting texture layer.
      /** The reflection map should be set as first texture. The
      transparency depends on the alpha value in the vertex colors. A
      texture which will not reflect can be set as second texture.
      Please note that this material type is currently not 100%
      implemented in OpenGL. */
      EMT_TRANSPARENT_REFLECTION_2_LAYER,

      //! A solid normal map renderer.
      /** First texture is the color map, the second should be the
      normal map. Note that you should use this material only when
      drawing geometry consisting of vertices of type
      S3DVertexTangents (EVT_TANGENTS). You can convert any mesh into
      this format using IMeshManipulator::createMeshWithTangents()
      (See SpecialFX2 Tutorial). This shader runs on vertex shader
      1.1 and pixel shader 1.1 capable hardware and falls back to a
      fixed function lighted material if this hardware is not
      available. Only two lights are supported by this shader, if
      there are more, the nearest two are chosen. */
      EMT_NORMAL_MAP_SOLID,

      //! A transparent normal map renderer.
      /** First texture is the color map, the second should be the
      normal map. Note that you should use this material only when
      drawing geometry consisting of vertices of type
      S3DVertexTangents (EVT_TANGENTS). You can convert any mesh into
      this format using IMeshManipulator::createMeshWithTangents()
      (See SpecialFX2 Tutorial). This shader runs on vertex shader
      1.1 and pixel shader 1.1 capable hardware and falls back to a
      fixed function lighted material if this hardware is not
      available. Only two lights are supported by this shader, if
      there are more, the nearest two are chosen. */
      EMT_NORMAL_MAP_TRANSPARENT_ADD_COLOR,

      //! A transparent (based on the vertex alpha value) normal map renderer.
      /** First texture is the color map, the second should be the
      normal map. Note that you should use this material only when
      drawing geometry consisting of vertices of type
      S3DVertexTangents (EVT_TANGENTS). You can convert any mesh into
      this format using IMeshManipulator::createMeshWithTangents()
      (See SpecialFX2 Tutorial). This shader runs on vertex shader
      1.1 and pixel shader 1.1 capable hardware and falls back to a
      fixed function lighted material if this hardware is not
      available.  Only two lights are supported by this shader, if
      there are more, the nearest two are chosen. */
      EMT_NORMAL_MAP_TRANSPARENT_VERTEX_ALPHA,

      //! Just like EMT_NORMAL_MAP_SOLID, but uses parallax mapping.
      /** Looks a lot more realistic. This only works when the
      hardware supports at least vertex shader 1.1 and pixel shader
      1.4. First texture is the color map, the second should be the
      normal map. The normal map texture should contain the height
      value in the alpha component. The
      IVideoDriver::makeNormalMapTexture() method writes this value
      automatically when creating normal maps from a heightmap when
      using a 32 bit texture. The height scale of the material
      (affecting the bumpiness) is being controlled by the
      SMaterial::MaterialTypeParam member. If set to zero, the
      default value (0.02f) will be applied. Otherwise the value set
      in SMaterial::MaterialTypeParam is taken. This value depends on
      with which scale the texture is mapped on the material. Too
      high or low values of MaterialTypeParam can result in strange
      artifacts. */
      EMT_PARALLAX_MAP_SOLID,

      //! A material like EMT_PARALLAX_MAP_SOLID, but transparent.
      /** Using EMT_TRANSPARENT_ADD_COLOR as base material. */
      EMT_PARALLAX_MAP_TRANSPARENT_ADD_COLOR,

      //! A material like EMT_PARALLAX_MAP_SOLID, but transparent.
      /** Using EMT_TRANSPARENT_VERTEX_ALPHA as base material. */
      EMT_PARALLAX_MAP_TRANSPARENT_VERTEX_ALPHA,

      //! BlendFunc = source * sourceFactor + dest * destFactor ( E_BLEND_FUNC )
      /** Using only first texture. Generic blending method. */
      EMT_ONETEXTURE_BLEND,

      //! This value is not used. It only forces this enumeration to compile to 32 bit.
      EMT_FORCE_32BIT = 0x7fffffff
    };

    //! These flags allow to define the interpretation of vertex color when lighting is enabled
    /** Without lighting being enabled the vertex color is the only value defining the fragment color.
    Once lighting is enabled, the four values for diffuse, ambient, emissive, and specular take over.
    With these flags it is possible to define which lighting factor shall be defined by the vertex color
    instead of the lighting factor which is the same for all faces of that material.
    The default is to use vertex color for the diffuse value, another pretty common value is to use
    vertex color for both diffuse and ambient factor. */
    enum E_COLOR_MATERIAL
    {
      //! Don't use vertex color for lighting
      ECM_NONE = 0,
      //! Use vertex color for diffuse light, this is default
      ECM_DIFFUSE,
      //! Use vertex color for ambient light
      ECM_AMBIENT,
      //! Use vertex color for emissive light
      ECM_EMISSIVE,
      //! Use vertex color for specular light
      ECM_SPECULAR,
      //! Use vertex color for both diffuse and ambient light
      ECM_DIFFUSE_AND_AMBIENT
    };

    struct SMaterial
    {
      E_MATERIAL_TYPE MaterialType;
      bool ZWriteEnable;
      bool Lighting;
      SColor DiffuseColor;
      SColor AmbientColor;
      E_COLOR_MATERIAL ColorMaterial;
      bool GouraudShading;
      bool BackfaceCulling;
      float Shininess;
    };
  }

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
//      virtual unsigned getJointCount() const = 0;

      //! Gets the name of a joint.
      /** \param number: Zero based index of joint. The last joint
      has the number getJointCount()-1;
      \return Name of joint and null if an error happened. */
//      virtual const char* getJointName(unsigned number) const = 0;

      //! Gets a joint number from its name
      /** \param name: Name of the joint.
      \return Number of the joint or -1 if not found. */
//      virtual int getJointNumber(const char* name) const = 0;

      //! Use animation from another mesh
      /** The animation is linked (not copied) based on joint names
      so make sure they are unique.
      \return True if all joints in this mesh were
      matched up (empty names will not be matched, and it's case
      sensitive). Unmatched joints will not be animated. */
//      virtual bool useAnimationFrom(const ISkinnedMesh *mesh) = 0;

      //! Update Normals when Animating
      /** \param on If false don't animate, which is faster.
      Else update normals, which allows for proper lighting of
      animated meshes. */
//      virtual void updateNormalsWhenAnimating(bool on) = 0;

      //! Sets Interpolation Mode
//      virtual void setInterpolationMode(E_INTERPOLATION_MODE mode) = 0;

      //! Animates this mesh's joints based on frame input
//      virtual void animateMesh(float frame, float blend) = 0;

      //! Preforms a software skin on this mesh based of joint positions
//      virtual void skinMesh(float strength = 1.f) = 0;

      //! converts the vertex type of all meshbuffers to tangents.
      /** E.g. used for bump mapping. */
//      virtual void convertMeshToTangents() = 0;

      //! Allows to enable hardware skinning.
      /* This feature is not implementated in Irrlicht yet */
//      virtual bool setHardwareSkinning(bool on) = 0;

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
      private:
        std::vector<std::pair<SMeshBufferLightMap, video::SMaterial> > LocalBuffers;
        public:


      //Interface for the mesh loaders (finalize should lock these functions, and they should have some prefix like loader_

      //these functions will use the needed arrays, set values, etc to help the loaders

      //! exposed for loaders: to add mesh buffers
          std::vector<std::pair<SMeshBufferLightMap, video::SMaterial> >& getMeshBuffers()
          {
            return LocalBuffers;
          }

      //! exposed for loaders: joints list
//      virtual std::vector<SJoint*>& getAllJoints() = 0;

      //! exposed for loaders: joints list
//      virtual std::vector<SJoint*>& getAllJoints() const = 0;

      //! loaders should call this after populating the mesh
//      virtual void finalize() = 0;

      //! Adds a new meshbuffer to the mesh, access it as last one
          std::pair<SMeshBufferLightMap, video::SMaterial>& addMeshBuffer()
          {
            LocalBuffers.push_back(std::make_pair(scene::SMeshBufferLightMap(), video::SMaterial()));
            return LocalBuffers.back();
          }

      //! Adds a new joint to the mesh, access it as last one
          SJoint* addJoint(SJoint *parent = 0)
          {
            SJoint *joint = new SJoint;

            //AllJoints.push_back(joint);
            return joint;
          }

      //! Adds a new weight to the mesh, access it as last one
          SWeight* addWeight(SJoint *joint)
          {
            joint->Weights.push_back(SWeight());
            return &joint->Weights.back();
          }

      //! Adds a new position key to the mesh, access it as last one
//      virtual SPositionKey* addPositionKey(SJoint *joint) = 0;
      //! Adds a new scale key to the mesh, access it as last one
//      virtual SScaleKey* addScaleKey(SJoint *joint) = 0;
      //! Adds a new rotation key to the mesh, access it as last one
//      virtual SRotationKey* addRotationKey(SJoint *joint) = 0;

      //! Check if the mesh is non-animated
//      virtual bool isStatic() = 0;
    };

  } // end namespace scene
} // end namespace irr

#endif

