#ifndef __SpecialManager_h_
#define __SpecialManager_h_

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

#include "SpecialInterface.h"
#include "SpecialJyn.h"
#include "SpecialPunch.h"
#include "SpecialKick.h"

class SpecialManager{
public:
	SpecialManager( Ogre::SceneManager* _sceneMgr, Robot* _robotSpeller,  OgreBulletDynamics::DynamicsWorld* _world );
	virtual ~SpecialManager(void);

	void update(const Ogre::Real time);
	void collision(btRigidBody *rigid, btRigidBody *rigid2);

	Robot* getTarget();
	void setTarget( Robot* _robotTarget );

	Robot* getSpeller();
	void setSpeller( Robot* _robotSpeller );

	void destroySpecial( SpecialInterface* special );

	/** MOVES */
	SpecialJyn* specialJyn( float _damage );
	SpecialPunch* specialPunch( float _damage );
	SpecialKick* specialKick( float _damage );

	Ogre::SceneNode 
		*specialNode;
protected:
private:
	std::list<SpecialInterface*> specialList;
	
	OgreBulletDynamics::DynamicsWorld *world;
	
	Ogre::SceneManager *sceneMgr;

	Robot
		*robotSpeller,
		*robotTarget;

	int count;

	/** Para os iterators*/
	SpecialInterface* _special;
	std::list<SpecialInterface*>::iterator itSpecialList;
	std::list<SpecialInterface*>::iterator itSpecialListEnd;
};

#endif // #ifndef __SpecialManager_h_