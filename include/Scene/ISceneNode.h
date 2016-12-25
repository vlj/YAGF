// Copyright (C) 2002-2012 Nikolaus Gebhardt
// Copyright (C) 2015 Vincent Lejeune
// Contains code from the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in License.txt

#pragma once

//#include "IAttributeExchangingObject.h"
//#include "ESceneNodeTypes.h"
//#include "ECullingTypes.h"
//#include "EDebugSceneTypes.h"
//#include "SMaterial.h"
//#include "irrString.h"
//#include <Maths/aabbox3d.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <list>
#include <string>
//#include "IAttributes.h"

namespace irr
{
	namespace scene
	{
		class ISceneManager;

		//! Typedef for list of scene nodes
		class ISceneNode;
		typedef std::list<ISceneNode*> ISceneNodeList;

		//! Scene node interface.
		/** A scene node is a node in the hierarchical scene graph. Every scene
		node may have children, which are also scene nodes. Children move
		relative to their parent's position. If the parent of a node is not
		visible, its children won't be visible either. In this way, it is for
		example easily possible to attach a light to a moving car, or to place
		a walking character on a moving platform on a moving ship.
		*/
		class ISceneNode
		{
		public:

			//! Constructor
			ISceneNode(ISceneNode* parent,
				const glm::vec3& position = glm::vec3(),
				const glm::vec3& rotation = glm::vec3(),
				const glm::vec3& scale = glm::vec3(1.0f, 1.0f, 1.0f))
				: RelativeTranslation(position), RelativeRotation(rotation), RelativeScale(scale),
				Parent(0),
				//        AutomaticCullingState(EAC_BOX), DebugDataVisible(EDS_OFF),
				IsVisible(true)
			{
				if (parent)
					parent->addChild(this);

				updateAbsolutePosition();
			}


			//! Destructor
			virtual ~ISceneNode()
			{
				// delete all children
				removeAll();
			}


			//! This method is called just before the rendering process of the whole scene.
			/** Nodes may register themselves in the render pipeline during this call,
			precalculate the geometry which should be renderered, and prevent their
			children from being able to register themselves if they are clipped by simply
			not calling their OnRegisterSceneNode method.
			If you are implementing your own scene node, you should overwrite this method
			with an implementation code looking like this:
			\code
			if (IsVisible)
			SceneManager->registerNodeForRendering(this);

			ISceneNode::OnRegisterSceneNode();
			\endcode
			*/
			virtual void OnRegisterSceneNode()
			{
				if (IsVisible)
				{
					ISceneNodeList::iterator it = Children.begin();
					for (; it != Children.end(); ++it)
						(*it)->OnRegisterSceneNode();
				}
			}


			//! OnAnimate() is called just before rendering the whole scene.
			/** Nodes may calculate or store animations here, and may do other useful things,
			depending on what they are. Also, OnAnimate() should be called for all
			child scene nodes here. This method will be called once per frame, independent
			of whether the scene node is visible or not.
			\param timeMs Current time in milliseconds. */
			virtual void OnAnimate(unsigned timeMs)
			{
				if (IsVisible)
				{
					// update absolute position
					updateAbsolutePosition();

					// perform the post render process on all children

					ISceneNodeList::iterator it = Children.begin();
					for (; it != Children.end(); ++it)
						(*it)->OnAnimate(timeMs);
				}
			}


			//! Renders the node.
			virtual void render() = 0;


			//! Returns the name of the node.
			/** \return Name as character string. */
			virtual const char* getName() const
			{
				return Name.c_str();
			}


			//! Sets the name of the node.
			/** \param name New name of the scene node. */
			virtual void setName(const char* name)
			{
				Name = name;
			}


			//! Sets the name of the node.
			/** \param name New name of the scene node. */
			virtual void setName(const std::string& name)
			{
				Name = name;
			}


			//! Get the axis aligned, not transformed bounding box of this node.
			/** This means that if this node is an animated 3d character,
			moving in a room, the bounding box will always be around the
			origin. To get the box in real world coordinates, just
			transform it with the matrix you receive with
			getAbsoluteTransformation() or simply use
			getTransformedBoundingBox(), which does the same.
			\return The non-transformed bounding box. */
			//      virtual const core::aabbox3d<f32>& getBoundingBox() const = 0;


				  //! Get the axis aligned, transformed and animated absolute bounding box of this node.
				  /** \return The transformed bounding box. */
			/*      virtual const core::aabbox3d<f32> getTransformedBoundingBox() const
				  {
					core::aabbox3d<f32> box = getBoundingBox();
					AbsoluteTransformation.transformBoxEx(box);
					return box;
				  }*/


				  //! Get the absolute transformation of the node. Is recalculated every OnAnimate()-call.
				  /** NOTE: For speed reasons the absolute transformation is not
				  automatically recalculated on each change of the relative
				  transformation or by a transformation change of an parent. Instead the
				  update usually happens once per frame in OnAnimate. You can enforce
				  an update with updateAbsolutePosition().
				  \return The absolute transformation matrix. */
			virtual const glm::mat4& getAbsoluteTransformation() const
			{
				return AbsoluteTransformation;
			}


			//! Returns the relative transformation of the scene node.
			/** The relative transformation is stored internally as 3
			vectors: translation, rotation and scale. To get the relative
			transformation matrix, it is calculated from these values.
			\return The relative transformation matrix. */
			virtual glm::mat4 getRelativeTransformation() const
			{
				return glm::scale(RelativeScale) * glm::translate(RelativeTranslation) *
					glm::eulerAngleYXZ(RelativeRotation.y, RelativeRotation.x, RelativeRotation.z);
			}


			//! Returns whether the node should be visible (if all of its parents are visible).
			/** This is only an option set by the user, but has nothing to
			do with geometry culling
			\return The requested visibility of the node, true means
			visible (if all parents are also visible). */
			virtual bool isVisible() const
			{
				return IsVisible;
			}

			//! Check whether the node is truly visible, taking into accounts its parents' visibility
			/** \return true if the node and all its parents are visible,
			false if this or any parent node is invisible. */
			virtual bool isTrulyVisible() const
			{
				if (!IsVisible)
					return false;

				if (!Parent)
					return true;

				return Parent->isTrulyVisible();
			}

			//! Sets if the node should be visible or not.
			/** All children of this node won't be visible either, when set
			to false. Invisible nodes are not valid candidates for selection by
			collision manager bounding box methods.
			\param isVisible If the node shall be visible. */
			virtual void setVisible(bool isVisible)
			{
				IsVisible = isVisible;
			}

			//! Adds a child to this scene node.
			/** If the scene node already has a parent it is first removed
			from the other parent.
			\param child A pointer to the new child. */
			virtual void addChild(ISceneNode* child)
			{
				if (child && (child != this))
				{
					child->remove(); // remove from old parent
					Children.push_back(child);
					child->Parent = this;
				}
			}


			//! Removes a child from this scene node.
			/** If found in the children list, the child pointer is also
			dropped and might be deleted if no other grab exists.
			\param child A pointer to the child which shall be removed.
			\return True if the child was removed, and false if not,
			e.g. because it couldn't be found in the children list. */
			virtual bool removeChild(ISceneNode* child)
			{
				ISceneNodeList::iterator it = Children.begin();
				for (; it != Children.end(); ++it)
					if ((*it) == child)
					{
						(*it)->Parent = 0;
						Children.erase(it);
						return true;
					}
				return false;
			}


			//! Removes all children of this scene node
			/** The scene nodes found in the children list are also dropped
			and might be deleted if no other grab exists on them.
			*/
			virtual void removeAll()
			{
				ISceneNodeList::iterator it = Children.begin();
				for (; it != Children.end(); ++it)
				{
					(*it)->Parent = 0;
				}

				Children.clear();
			}


			//! Removes this scene node from the scene
			/** If no other grab exists for this node, it will be deleted.
			*/
			virtual void remove()
			{
				if (Parent)
					Parent->removeChild(this);
			}

			//! Gets the scale of the scene node relative to its parent.
			/** This is the scale of this node relative to its parent.
			If you want the absolute scale, use
			getAbsoluteTransformation().getScale()
			\return The scale of the scene node. */
			virtual const glm::vec3& getScale() const
			{
				return RelativeScale;
			}

			//! Sets the relative scale of the scene node.
			/** \param scale New scale of the node, relative to its parent. */
			virtual void setScale(const glm::vec3& scale)
			{
				RelativeScale = scale;
			}


			//! Gets the rotation of the node relative to its parent.
			/** Note that this is the relative rotation of the node.
			If you want the absolute rotation, use
			getAbsoluteTransformation().getRotation()
			\return Current relative rotation of the scene node. */
			virtual const glm::vec3& getRotation() const
			{
				return RelativeRotation;
			}


			//! Sets the rotation of the node relative to its parent.
			/** This only modifies the relative rotation of the node.
			\param rotation New rotation of the node in degrees. */
			virtual void setRotation(const glm::vec3& rotation)
			{
				RelativeRotation = rotation;
			}


			//! Gets the position of the node relative to its parent.
			/** Note that the position is relative to the parent. If you want
			the position in world coordinates, use getAbsolutePosition() instead.
			\return The current position of the node relative to the parent. */
			virtual const glm::vec3& getPosition() const
			{
				return RelativeTranslation;
			}

			//! Sets the position of the node relative to its parent.
			/** Note that the position is relative to the parent.
			\param newpos New relative position of the scene node. */
			virtual void setPosition(const glm::vec3& newpos)
			{
				RelativeTranslation = newpos;
			}

			//! Gets the absolute position of the node in world coordinates.
			/** If you want the position of the node relative to its parent,
			use getPosition() instead.
			NOTE: For speed reasons the absolute position is not
			automatically recalculated on each change of the relative
			position or by a position change of an parent. Instead the
			update usually happens once per frame in OnAnimate. You can enforce
			an update with updateAbsolutePosition().
			\return The current absolute position of the scene node (updated on last call of updateAbsolutePosition). */
			virtual glm::vec3 getAbsolutePosition() const
			{
				return glm::vec3(AbsoluteTransformation[3]);
			}


			//! Enables or disables automatic culling based on the bounding box.
			/** Automatic culling is enabled by default. Note that not
			all SceneNodes support culling and that some nodes always cull
			their geometry because it is their only reason for existence,
			for example the OctreeSceneNode.
			\param state The culling state to be used. */
			/*      void setAutomaticCulling(u32 state)
				  {
					AutomaticCullingState = state;
				  }*/


				  //! Gets the automatic culling state.
				  /** \return The automatic culling state. */
			/*      u32 getAutomaticCulling() const
				  {
					return AutomaticCullingState;
				  }*/


				  //! Returns a const reference to the list of all children.
				  /** \return The list of all children of this node. */
			const std::list<ISceneNode*>& getChildren() const
			{
				return Children;
			}

			//! Returns a reference to the list of all children.
			/** \return The list of all children of this node. */
			std::list<ISceneNode*>& getChildren()
			{
				return Children;
			}


			//! Changes the parent of the scene node.
			/** \param newParent The new parent to be used. */
			virtual void setParent(ISceneNode* newParent)
			{
				remove();

				Parent = newParent;

				if (Parent)
					Parent->addChild(this);
			}


			//! Updates the absolute position based on the relative and the parents position
			/** Note: This does not recursively update the parents absolute positions, so if you have a deeper
			hierarchy you might want to update the parents first.*/
			virtual void updateAbsolutePosition()
			{
				if (Parent)
				{
					AbsoluteTransformation =
						Parent->getAbsoluteTransformation() * getRelativeTransformation();
				}
				else
					AbsoluteTransformation = getRelativeTransformation();
			}


			//! Returns the parent of this scene node
			/** \return A pointer to the parent. */
			scene::ISceneNode* getParent() const
			{
				return Parent;
			}

		protected:
			//! Name of the scene node.
			std::string Name;

			//! Absolute transformation of the node.
			glm::mat4 AbsoluteTransformation;

			//! Relative translation of the scene node.
			glm::vec3 RelativeTranslation;

			//! Relative rotation of the scene node.
			glm::vec3 RelativeRotation;

			//! Relative scale of the scene node.
			glm::vec3 RelativeScale;

			//! Pointer to the parent
			ISceneNode* Parent;

			//! List of all children of this node
			std::list<ISceneNode*> Children;

			//! Automatic culling state
	  //      u32 AutomaticCullingState;

			//! Is the node visible?
			bool IsVisible;
		};


	} // end namespace scene
} // end namespace irr
