#ifndef __BaseApplication_h_
#define __BaseApplication_h_

#include <Ogre.h>
#include <OgreApplicationContext.h>
#include <OgreInput.h>
#include <OgreTrays.h>
#include <OgreCameraMan.h>
#include <OgreOverlaySystem.h>

#include <map>
#include <string>
#include <ctime>

#include "ConfigScript.h"
#include "CollisionDetectionListener.h"
#include "Physics.h"

using namespace std;

/**
 * Application shell on top of OgreBites::ApplicationContext (Ogre 14): it creates the Root, the window
 * (SDL2), the RTSS shader generator and the resources, and forwards SDL2 input to the game's listeners.
 *
 * Game objects register as input listeners by name (addKeyListener/addMouseListener/addJoystickListener
 * all register the same OgreBites::InputListener; a listener registered twice is called once) and are
 * called in name order, like the 2011 OIS-based version.
 */
class BaseApplication: public OgreBites::ApplicationContext, public OgreBites::InputListener{
public:
	BaseApplication(void);
	virtual ~BaseApplication(void);

	virtual void go(void);

	Ogre::SceneManager* mSceneMgr;
	Ogre::Root *mRoot;
	Ogre::RenderWindow* mWindow;
	Ogre::Camera* mCamera;

	/// Per-user folder for ogre.cfg, logs and screenshots (%USERPROFILE%/Tumbu on Windows).
	Ogre::String workPath;

	void addKeyListener( OgreBites::InputListener *listener, const std::string& instanceName );
	void addMouseListener( OgreBites::InputListener *listener, const std::string& instanceName );
	void addJoystickListener( OgreBites::InputListener *listener, const std::string& instanceName );
	void addCollisionDetectionListener( CollisionDetectionListener *collisionListener, const std::string& instanceName );
	void addCollisionDetectionListener( CollisionDetectionListener *collisionListener );

	void removeMouseListener( const std::string& instanceName );
	void removeMouseListener( OgreBites::InputListener *listener );
	void removeKeyListener( const std::string& instanceName );
	void removeKeyListener( OgreBites::InputListener *listener );
	void removeJoystickListener( const std::string& instanceName );
	void removeJoystickListener( OgreBites::InputListener *listener );

	void removeCollisionDetectionListener( const std::string& instanceName );
	void removeCollisionDetectionListener( CollisionDetectionListener *collisionDetectionListener );
	CollisionDetectionListener *getCollisionListenerByName( const std::string& instanceName );

	void removeAllListeners( void );
	void removeAllKeyListeners( void );
	void removeAllMouseListeners( void );
	void removeAllJoystickListeners( void );
	void removeAllCollisionDetectionListeners( void );

	/// Hides the OS cursor and captures the mouse (relative motion) while playing.
	void setMouseCaptured( bool captured );
	OgreBites::TrayManager* getTrayManager(void){ return mTrayMgr; }

protected:
	virtual void setup(void);
	virtual void locateResources(void);
	virtual void loadResources(void);
	virtual void chooseSceneManager(void);
	virtual void createCamera(void);
	virtual void createFrameListener(void);
	virtual void createScene(void) = 0; // Must be overridden
	virtual void destroyScene(void);
	virtual void createViewports(void);

	virtual bool frameRenderingQueued( const Ogre::FrameEvent &evt );
	virtual void windowResized( Ogre::RenderWindow* rw );

	// Input from OgreBites, forwarded to the registered listeners.
	virtual bool keyPressed( const OgreBites::KeyboardEvent &evt );
	virtual bool keyReleased( const OgreBites::KeyboardEvent &evt );
	virtual bool mouseMoved( const OgreBites::MouseMotionEvent &evt );
	virtual bool mouseWheelRolled( const OgreBites::MouseWheelEvent &evt );
	virtual bool mousePressed( const OgreBites::MouseButtonEvent &evt );
	virtual bool mouseReleased( const OgreBites::MouseButtonEvent &evt );
	virtual bool textInput( const OgreBites::TextInputEvent &evt );
	virtual bool axisMoved( const OgreBites::AxisEvent &evt );
	virtual bool buttonPressed( const OgreBites::ButtonEvent &evt );
	virtual bool buttonReleased( const OgreBites::ButtonEvent &evt );

	OgreBites::TrayManager*	mTrayMgr;
	OgreBites::CameraMan*	mCameraMan;
	OgreBites::ParamsPanel*	mDetailsPanel;
	Ogre::SceneNode*		mCameraNode;
	Ogre::Viewport*			mViewport;
	bool mShutDown;

private:
	std::vector<OgreBites::InputListener*> listenersSnapshot(void) const;
	bool isListening( OgreBites::InputListener *listener ) const;
	void addInputListenerByName( OgreBites::InputListener *listener, const std::string& instanceName );
	void removeInputListenerByName( const std::string& instanceName );
	void removeInputListenerByPointer( OgreBites::InputListener *listener );

	std::map<std::string, OgreBites::InputListener*> mInputListeners;
	std::map<std::string, CollisionDetectionListener*> mCollisionDetectionListeners;
};

#endif // #ifndef __BaseApplication_h_
