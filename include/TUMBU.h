#ifndef __TUMBU_h_
#define __TUMBU_h_

#include "BaseApplication.h"
#include "Enums.h"

#include "Camera.h"
#include "Character.h"
#include "CharacterEnemy.h"
#include "Clock.h"
#include "Demo.h"
#include "GUI.h"
#include "Util.h"
#include "SoundManager.h"
#include "CutScene.h"
#include "StartScreen.h"
#include "AIManager.h"
#include "Sound.h"
#include "Tutorial.h"
#include "SimpleRigidBody.h"
	
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include "../res/resource.h"
#endif

class Clock;
class GUI;
class Demo;

class TUMBU : public BaseApplication{
public:
	TUMBU(void);
	virtual ~TUMBU(void);
	static TUMBU* getInstance(void);

	void initializeDemo(void);
	void finishDemo(void);
	void shutdown(void);
	bool isPlaying(void);

	void renderOneFrame(void);
	Ogre::Root* getRoot(void);

	SimpleRigidBody* TUMBU::createSimpleRigidBody(OgreBulletDynamics::DynamicsWorld* physicWorld, const Ogre::String &instanceName, const Ogre::String &meshName, const Ogre::Vector3 &pos, const Ogre::Quaternion &q, const Ogre::Real bodyRestitution, const Ogre::Real bodyFriction, bool shadows, TumbuEnums::PhysicObjectTag tag);
	SimpleRigidBody* TUMBU::createSimpleRigidBody(OgreBulletDynamics::DynamicsWorld* physicWorld, Ogre::Entity* entity, Ogre::SceneNode* node, const Ogre::Vector3 &pos, const Ogre::Quaternion &q, const Ogre::Real bodyRestitution, const Ogre::Real bodyFriction, bool shadows, TumbuEnums::PhysicObjectTag tag);
	
	/*************************************************************/
	/******************** GETTERS AND SETTERS ********************/
	void setGameState( TumbuEnums::GameState _gameState );
	TumbuEnums::GameState getGameState(void);

	void setActiveSceneNode( Ogre::SceneNode* _activeScene );
	Ogre::SceneNode* getActiveSceneNode(void);

	void setAIManager( AIManager* _ai );
	AIManager* getAIManager(void);

	void setCastShadows( bool _castShadows );
	bool isCastShadows(void);

	void setShadowFarDistance( int _shadowFarDistance );
	int getShadowFarDistance(void);

	void setShadowTextureSize( int _shadowTextureSize );
	int getShadowTextureSize(void);

	void setShadowTextureCount( int _shadowTextureCount );
	int getShadowTextureCount();

	void setSkyQuality( int _skyQuality );
	int getSkyQuality(void);

	void setShadowTechnique( Ogre::ShadowTechnique _shadowTechnique );
	Ogre::ShadowTechnique getShadowTechnique();
	
	void setShadowColor( Ogre::ColourValue _shadowColor );
	Ogre::ColourValue getShadowColor(void);

	void setTimeMultiplier( Ogre::Real _timeMultiplier );
	Ogre::Real getTimeMultiplier(void);

	void setClock( Clock* _clock );
	Clock* getClock(void);

	Demo* getDemo(void);

	void printSceneHierarchy(void);
protected:
	void createScene(void);
	void createStartMenu(void);

    bool frameRenderingQueued( const Ogre::FrameEvent& evt );
	bool frameStarted( const Ogre::FrameEvent& evt );

	bool keyPressed( const OIS::KeyEvent &arg );
	bool keyReleased( const OIS::KeyEvent &arg );
	bool mouseMoved( const OIS::MouseEvent &arg );
	bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
	bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );

    bool povMoved( const OIS::JoyStickEvent &e, int pov );
    bool axisMoved( const OIS::JoyStickEvent &e, int axis );
    bool sliderMoved( const OIS::JoyStickEvent &e, int sliderID );
    bool buttonPressed( const OIS::JoyStickEvent &e, int button );
    bool buttonReleased( const OIS::JoyStickEvent &e, int button );	
private:
	void initializeGUIStuff(void);
	void initializeUtil(void);
	void printSceneChildren(Ogre::SceneNode* node, int level);

	Ogre::ShadowTechnique shadowTechnique;
	Ogre::ColourValue shadowColor;
	Ogre::Real timeMultiplier;

	bool castShadows;

	int barrelIndex, 
		mNumEntitiesInstanced,
		
		shadowFarDistance,
		shadowTextureSize,
		shadowTextureCount,
		skyQuality;

	OgreBulletCollisions::StaticMeshToShapeConverter		*trimeshConverter;
	OgreBulletCollisions::AnimatedMeshToShapeConverter		*animConverter;

	AIManager				*ai;
	Sound					*startScreenBackgroundSound;
	Clock					*clock;
	Demo					*demo;
	TumbuEnums::GameState	gameState;
	GUI						*gui;
	SoundManager			*soundManager;
	StartScreen				*cutScene;

	Ogre::SceneNode
		*activeScene;

	static TUMBU* instance;
};
#endif // #ifndef __TUMBU_h_