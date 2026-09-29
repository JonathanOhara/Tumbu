#ifndef __SimpleRigidBody_h_
#define __SimpleRigidBody_h_

#include <iostream>

#include <Ogre.h>
#include <math.h>

#include "Physics.h"


#include "CollisionDetectionListener.h"

using namespace std;

class SimpleRigidBody: public CollisionDetectionListener {
public:
	SimpleRigidBody( Ogre::SceneManager *_sceneManager, TumbuEnums::PhysicObjectTag _objectTag );
	virtual ~SimpleRigidBody(void);

	Physics::RigidBody *getOgreBulletRigidBody( const std::string& instanceName );
	void setOgreBulletRigidBody( Physics::RigidBody *_rigidBody );

	void collision( CollisionDetectionListener *other );

	Physics::CollisionShape *shape;

	Ogre::SceneNode *node;
	Ogre::Entity *entity;

protected:

private:
	Ogre::SceneManager *sceneManager;

	Physics::RigidBody *rigidBody;
};

#endif // #ifndef __SimpleRigidBody_h_