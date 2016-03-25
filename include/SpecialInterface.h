#ifndef __SpecialInterface_h_
#define __SpecialInterface_h_

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

#include "EnergyParticle.h"
#include "CollisionDetectionListener.h"

class Robot;

#define NUMBER_OF_PARTICLES 10

#define AC1 0.5f
#define AC2	0.5f

#define PARTICLE_WIDTH 0.25f
#define PARTICLE_HEIGHT 0.25f
#define PARTICLE_COLOR Ogre::ColourValue(0.93f, 0.11f, 0.14f, 1)
#define PARTICLE_LIVE_TIME 600
#define PARTICLE_MATERIAL "PE/energy"

#define PARTICLE_LIGHT_DIFFUSE_COLOR Ogre::ColourValue::White
#define PARTICLE_LIGHT_SPECULAR_COLOR Ogre::ColourValue::White

class SpecialInterface: public CollisionDetectionListener{
public:
	SpecialInterface(void);
	virtual ~SpecialInterface(void);

	virtual void update(const Ogre::Real time) = 0;

	OgreBulletDynamics::RigidBody *getOgreBulletRigidBody( const std::string& instanceName ) = 0;
	void collision( CollisionDetectionListener *other ) = 0;

	virtual void concentrate() = 0;
	virtual void attack(Ogre::Quaternion orientation) = 0;
	virtual void hitScenario( Ogre::Vector3 position ) = 0;
	virtual void hit(Ogre::SceneNode* hittedNode) = 0;
	virtual void toDelete() = 0;
	virtual void clear() = 0;
	
	Robot* getTarget();
	void setTarget( Robot* _robotTarget );

	Robot* getSpeller();
	void setSpeller( Robot* _robotSpeller );

	enum SpecialStatus { NONE, CONCENTRATING, CONCENTRATED, ATTACKING, HITTED, FADEOUT, FINISHED, TO_DELETE };

	SpecialStatus getSpecialStatus();
	void setSpecialStatus( SpecialStatus _specialStatus );

	std::list<OgreBulletDynamics::RigidBody*> specialRigidBodyList;
	std::vector<EnergyParticle*> particleList;

	float damage;
protected:
	Ogre::SceneManager *sceneMgr;
	OgreBulletDynamics::DynamicsWorld *world;
	OgreBulletCollisions::BoxCollisionShape	*specialShape;

	Robot
		*robotSpeller,
		*robotTarget;
	
	Ogre::ParticleSystem
		*explosionParticleSystem,
		*particleSystem;
	
	Ogre::SceneNode 
		*specialRigidNode,
		*particleSystemNode;

	Ogre::Light *specialLight;

	SpecialStatus specialStatus;
	
	int count;
private:
};

#endif // #ifndef __SpecialInterface_h_