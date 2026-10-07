#ifndef __CollisionDetectionListener_h_
#define __CollisionDetectionListener_h_

#include <iostream>

#include <Ogre.h>
#include <math.h>

#include "Physics.h"


#include "Enums.h"

using namespace std;

class CollisionDetectionListener {
public:
	CollisionDetectionListener( );
	virtual ~CollisionDetectionListener(void);

	virtual Physics::RigidBody *getOgreBulletRigidBody( const std::string& instanceName ) = 0;
	virtual void collision( CollisionDetectionListener *other ) = 0;

	TumbuEnums::PhysicObjectTag objectTag;

	//Esse parametro só tem valor na detecção de colisão!
	std::string rigidBodyName;

	Ogre::Vector3 collisionPosition;
protected:

private:
};

#endif // #ifndef __CollisionDetectionListener_h_