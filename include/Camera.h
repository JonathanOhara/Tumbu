#ifndef __Camera_h_
#define __Camera_h_

#include <Ogre.h>
#include <OIS.h>

#include <SdkTrays.h>
#include <SdkCameraMan.h>

#include "GameObject.h"

class Camera: public OIS::KeyListener, public OIS::JoyStickListener, public OIS::MouseListener, public Ogre::FrameListener{
public:
	enum CameraAnimation{ NONE, ROTATING_LEFT, ROTATING_RIGHT, ZOOM_IN, ZOOM_OUT, ROTATING_UP, ROTATING_DOWN };

	Camera( Ogre::Camera* camera, Ogre::SceneNode* mChaseNode );
	virtual ~Camera(void);

	bool frameRenderingQueued(const Ogre::FrameEvent &evt);
	bool keyPressed( const OIS::KeyEvent &arg );
	bool keyReleased( const OIS::KeyEvent &arg );

	bool mouseMoved(const OIS::MouseEvent &arg);
	bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);

    bool povMoved( const OIS::JoyStickEvent &e, int pov );
    bool axisMoved( const OIS::JoyStickEvent &e, int axis );
    bool sliderMoved( const OIS::JoyStickEvent &e, int sliderID );
    bool buttonPressed( const OIS::JoyStickEvent &e, int button );
    bool buttonReleased( const OIS::JoyStickEvent &e, int button );

	void updateCameraGoal( Ogre::Real deltaYaw, Ogre::Real deltaPitch, Ogre::Real deltaZoom );

	void startExplorationMode();
	void startFightMode(Ogre::SceneNode* mLockNode);

	CameraAnimation getAnimation();

	Ogre::Camera* mCamera;
	
	Ogre::SceneNode* chaseNode;
	Ogre::SceneNode* lockNode;
	Ogre::SceneNode 
		*mCameraNode, 
		*mCameraPivot,
		*mCameraGoal;
protected:

private:
	void initiateCameraPosition(void);
	Ogre::Quaternion startOrientation;

	CameraAnimation animation;

	int screenFull;
	
	Ogre::Real mPivotPitch;

	Ogre::Real 
		cameraZDistance,
		cameraHeight,
		cameraTranslate,
		maxBoundPitch,
		minBoundPitch,
		maxBoundZoom,
		minBoundZoom;

	float rotationFactor;

	bool animatedByMouse;

	int counter;
};

#endif // #ifndef __Camera_h_