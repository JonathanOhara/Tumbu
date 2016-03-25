#ifndef __Demo_h_
#define __Demo_h_

#include <Ogre.h>

#include <Terrain/OgreTerrain.h>
#include <Terrain/OgreTerrainGroup.h>
#include <Terrain/OgreTerrainMaterialGeneratorA.h>

#include <OIS.h>
#include <iostream>

#include "CollisionDetectionListener.h"
#include "ConfigScript.h"
#include "Character.h"
#include "CharacterEnemy.h"
#include "GUI.h"
#include "TUMBU.h"
#include "Tutorial.h"
#include "Sky.h"
#include "SimpleRigidBody.h"

#include "DotSceneLoader.h"

using namespace std;

class GUI;
class Sky;
class TUMBU;
class DotSceneLoader;

class Demo: public Ogre::FrameListener{
public:
	Demo();
	virtual ~Demo(void);

	bool frameRenderingQueued(const Ogre::FrameEvent &evt);

	void initializeDemo();

	void startTutorialMode(void);
	void stopTutorialMode(void);

	/** CHAR */
	Character* mainChar;
	CharacterEnemy* enemy;

	Ogre::SceneNode
		*demoScene;

	int TOTAL_ENEMIES;
	int numberOfenemies;

	void setCamera( Camera* _camera );
	Camera* getCamera();

	void setPhysicWorld( OgreBulletDynamics::DynamicsWorld* _physicWorld );
	OgreBulletDynamics::DynamicsWorld* getPhysicWorld();

	void initialiseGameResources(void);

protected:

private:
	void collisionDetection(void);

	void createLightEffects(void);
	void initializePhysicsStuff(void);
	void createSky(void);
	void createSimpleTerrain(void);
	void createTerrainPhysic(void);
	void createArena(void);
	void createMainCharacter(void);
	void createEnemyCharacter(void);
	void setupCamera(void);
	void startInitialConversation(void);

	void verifyDeaths(void);
	void heroDie(void);
	void enemyDie(void);
	
	OgreBulletCollisions::CollisionShape	*defaultPlaneShape;
	
	OgreBulletDynamics::RigidBody
		*defaultPlaneBody;

	
	float * pDataConvert;

	SimpleRigidBody
		*arenaRigidBody,
		*colliseumRigidBody,
		*floorRigidBody,
		*terrainRigidBody;

	OgreBulletDynamics::DynamicsWorld* physicWorld;
#ifdef _DEBUG
    OgreBulletCollisions::DebugDrawer* debugDrawer;
	Ogre::SceneNode *debugDrawerNode;
#endif

	Camera			*camera;
	GUI				*gui;
	Sound			*backGroundSound;
	SoundManager	*soundManager;
	Sky				*sky;
	TUMBU			*tumbu;
	Tutorial		*tutorial;

	DotSceneLoader* mLoader;

	Ogre::SceneManager* mSceneMgr;
};

#endif // #ifndef __Demo_h_