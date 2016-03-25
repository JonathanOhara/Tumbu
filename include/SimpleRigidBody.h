#ifndef __SimpleRigidBody_h_
#define __SimpleRigidBody_h_

#include <iostream>

#include <Ogre.h>
#include <OIS.h>
#include <math.h>

/** COLLISIONS */
#include <Utils/OgreBulletConverter.h>
#include <Debug/OgreBulletCollisionsDebugLines.h>
#include <Debug/OgreBulletCollisionsDebugShape.h>
#include <Debug/OgreBulletCollisionsDebugDrawer.h>
#include <Debug/OgreBulletCollisionsDebugContact.h>
#include <OgreBulletCollisionsShape.h>
#include <Shapes/OgreBulletCollisionsBoxShape.h>
#include <Shapes/OgreBulletCollisionsConeShape.h>
#include <Shapes/OgreBulletCollisionsSphereShape.h>
#include <Shapes/OgreBulletCollisionsCylinderShape.h>
#include <Shapes/OgreBulletCollisionsTriangleShape.h>
#include <Shapes/OgreBulletCollisionsTerrainShape.h>
#include <Shapes/OgreBulletCollisionsCylinderShape.h>
#include <Shapes/OgreBulletCollisionsCapsuleShape.h>
#include <Shapes/OgreBulletCollisionsStaticPlaneShape.h>
#include <Shapes/OgreBulletCollisionsCompoundShape.h>
#include <Shapes/OgreBulletCollisionsMultiSphereShape.h>
#include <Shapes/OgreBulletCollisionsConvexHullShape.h>
#include <Shapes/OgreBulletCollisionsMinkowskiSumShape.h>
#include <Shapes/OgreBulletCollisionsTrimeshShape.h>
#include <Shapes/OgreBulletCollisionsGImpactShape.h>
#include <Utils/OgreBulletCollisionsMeshToShapeConverter.h>

/** DYNAMICS */
#include <OgreBulletDynamicsRigidBody.h>
#include <OgreBulletDynamicsObjectState.h>
#include <OgreBulletDynamicsConstraint.h>
#include <Constraints/OgreBulletDynamics6DofConstraint.h>
#include <Constraints/OgreBulletDynamicsHingeConstraint.h>
#include <Constraints/OgreBulletDynamicsPoint2pointConstraint.h>
#include <Constraints/OgreBulletDynamicsConeTwistConstraint.h>
#include <Constraints/OgreBulletDynamicsRaycastVehicle.h>
#include <Prefab/OgreBulletDynamicsRagDoll.h>
#include <OgreBulletDynamicsWorld.h>

#include "CollisionDetectionListener.h"

using namespace std;

class SimpleRigidBody: public CollisionDetectionListener {
public:
	SimpleRigidBody( Ogre::SceneManager *_sceneManager, TumbuEnums::PhysicObjectTag _objectTag );
	virtual ~SimpleRigidBody(void);

	OgreBulletDynamics::RigidBody *getOgreBulletRigidBody( const std::string& instanceName );
	void setOgreBulletRigidBody( OgreBulletDynamics::RigidBody *_rigidBody );

	void collision( CollisionDetectionListener *other );

	OgreBulletCollisions::CollisionShape *shape;

	Ogre::SceneNode *node;
	Ogre::Entity *entity;

protected:

private:
	Ogre::SceneManager *sceneManager;

	OgreBulletDynamics::RigidBody *rigidBody;
};

#endif // #ifndef __SimpleRigidBody_h_