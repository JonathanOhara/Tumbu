#ifndef __BaseApplication_h_
#define __BaseApplication_h_

#include <OgreCamera.h>
#include <OgreEntity.h>
#include <OgreLogManager.h>
#include <OgreRoot.h>
#include <OgreViewport.h>
#include <OgreSceneManager.h>
#include <OgreRenderWindow.h>
#include <OgreConfigFile.h>

#include <OISEvents.h>
#include <OISInputManager.h>
#include <OISKeyboard.h>
#include <OISMouse.h>
#include <OISJoyStick.h>

#include <SdkTrays.h>
#include <SdkCameraMan.h>

#include <CEGUI.h> 
#include <RendererModules/Ogre/CEGUIOgreRenderer.h>

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

#include <CEGUI.h> 
#include <RendererModules/Ogre/CEGUIOgreRenderer.h>

#include <ctime>

#include "ConfigScript.h"
#include "CollisionDetectionListener.h"

using namespace std;

class BaseApplication : public Ogre::FrameListener, public Ogre::WindowEventListener, public OIS::KeyListener, public OIS::MouseListener, public OIS::JoyStickListener, OgreBites::SdkTrayListener{
public:
	BaseApplication(void);
	virtual ~BaseApplication(void);

	virtual void go(void);

	Ogre::SceneManager* mSceneMgr;
	Ogre::Root *mRoot;
	Ogre::RenderWindow* mWindow;
	Ogre::Camera* mCamera;

	Ogre::String workPath;

    void addKeyListener( OIS::KeyListener *keyListener, const std::string& instanceName );
    void addMouseListener( OIS::MouseListener *mouseListener, const std::string& instanceName );
	void addJoystickListener( OIS::JoyStickListener *joystickListener, const std::string& instanceName );
	void addCollisionDetectionListener( CollisionDetectionListener *collisionListener, const std::string& instanceName );
	void addCollisionDetectionListener( CollisionDetectionListener *collisionListener );

    void removeMouseListener( const std::string& instanceName );
    void removeMouseListener( OIS::MouseListener *mouseListener );
	
	void removeKeyListener( const std::string& instanceName );
	void removeKeyListener( OIS::KeyListener *keyListener );
    
	void removeJoystickListener( const std::string& instanceName );
	void removeJoystickListener( OIS::JoyStickListener *joystickListener );

	void removeCollisionDetectionListener( const std::string& instanceName );
    void removeCollisionDetectionListener( CollisionDetectionListener *collisionDetectionListener );

	CollisionDetectionListener *getCollisionListenerByName( const std::string& instanceName );

    void removeAllListeners( void );
    void removeAllKeyListeners( void );
    void removeAllMouseListeners( void );
	void removeAllJoystickListeners( void );
	void removeAllCollisionDetectionListeners( void );
protected:
	virtual bool setup(void);
	virtual bool configure(void);
	virtual void chooseSceneManager(void);
	virtual void createCamera(void);
	virtual void createFrameListener(void);
	virtual void createScene(void) = 0; // Must be Override!
	virtual void destroyScene(void);
	virtual void createViewports(void);
	virtual void setupResources(void);
	virtual void createResourceListener(void);
	virtual void loadResources(void);
	virtual bool frameRenderingQueued(const Ogre::FrameEvent &evt);
	
	virtual bool keyPressed( const OIS::KeyEvent &arg );
	virtual bool keyReleased( const OIS::KeyEvent &arg );
	virtual bool mouseMoved( const OIS::MouseEvent &arg );
	virtual bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
	virtual bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );

    virtual bool povMoved( const OIS::JoyStickEvent &e, int pov );
    virtual bool axisMoved( const OIS::JoyStickEvent &e, int axis );
    virtual bool sliderMoved( const OIS::JoyStickEvent &e, int sliderID );
    virtual bool buttonPressed( const OIS::JoyStickEvent &e, int button );
    virtual bool buttonReleased( const OIS::JoyStickEvent &e, int button );

	//Adjust mouse clipping area
	virtual void windowResized(Ogre::RenderWindow* rw);
	//Unattach OIS before window shutdown (very important under Linux)
	virtual void windowClosed(Ogre::RenderWindow* rw);

	Ogre::String mResourcesCfg;
	Ogre::String mPluginsCfg;
	Ogre::Viewport* mViewport;

	// OgreBites
	OgreBites::SdkTrayManager*	mTrayMgr;
	OgreBites::SdkCameraMan* mCameraMan;     	// basic camera controller
	OgreBites::ParamsPanel* mDetailsPanel;   	// sample details panel
	bool mCursorWasVisible;						// was cursor visible before dialog appeared
	bool mShutDown;

	//OIS Input devices
	OIS::InputManager* mInputManager;

	OIS::Mouse*    mMouse;
	OIS::Keyboard* mKeyboard;
    std::vector<OIS::JoyStick*> mJoysticks;

	std::map<std::string, CollisionDetectionListener*> mCollisionDetectionListeners;
	std::map<std::string, OIS::JoyStickListener*> mJoystickListeners;
	std::map<std::string, OIS::KeyListener*> mKeyListeners;
    std::map<std::string, OIS::MouseListener*> mMouseListeners;

	std::map<std::string, CollisionDetectionListener*>::iterator itCollisionDetectionListener;
	std::map<std::string, OIS::JoyStickListener*>::iterator itJoystickListener;
	std::map<std::string, OIS::KeyListener*>::iterator itKeyListener;
    std::map<std::string, OIS::MouseListener*>::iterator itMouseListener;

	std::map<std::string, CollisionDetectionListener*>::iterator itCollisionDetectionListenerEnd;
	std::map<std::string, OIS::JoyStickListener*>::iterator itJoystickListenerEnd;
    std::map<std::string, OIS::KeyListener*>::iterator itKeyListenerEnd;
    std::map<std::string, OIS::MouseListener*>::iterator itMouseListenerEnd;
};

#endif // #ifndef __BaseApplication_h_