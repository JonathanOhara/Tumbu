#ifndef __Util_h_
#define __Util_h_

#include <Ogre.h>

#include "Physics.h"



#include "Robot.h"
#include "Part.h"

class Util{
public:
	Util(void);
	virtual ~Util(void);
	
	static Physics::RigidBody* createBarrel(int barrelIndex, Ogre::SceneManager* mSceneMgr, Physics::DynamicsWorld* world, Ogre::Camera* mCamera);
	static void updateShapeFromEntity(Robot *charUp, Part* part);

	static Ogre::SceneManager* mSceneMgr;
	static Physics::DynamicsWorld* world;
	static Ogre::Camera* mCamera;

protected:

private: 
};
#endif // #ifndef __Util_h_