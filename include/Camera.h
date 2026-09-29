#ifndef __Camera_h_
#define __Camera_h_

#include <Ogre.h>

#include "GameObject.h"

class Camera: public OgreBites::InputListener, public Ogre::FrameListener{
public:
	enum CameraAnimation{ NONE, ROTATING_LEFT, ROTATING_RIGHT, ZOOM_IN, ZOOM_OUT, ROTATING_UP, ROTATING_DOWN };

	Camera( Ogre::Camera* camera, Ogre::SceneNode* mChaseNode );
	virtual ~Camera(void);

	bool frameRenderingQueued(const Ogre::FrameEvent &evt);
	bool keyPressed( const OgreBites::KeyboardEvent &arg );
	bool keyReleased( const OgreBites::KeyboardEvent &arg );

	bool mouseMoved(const OgreBites::MouseMotionEvent &arg);
	bool mouseWheelRolled(const OgreBites::MouseWheelEvent &arg);
	bool mousePressed(const OgreBites::MouseButtonEvent &arg);
	bool mouseReleased(const OgreBites::MouseButtonEvent &arg);

    bool hatMoved( const OgreBites::HatEvent &e );
    bool axisMoved( const OgreBites::AxisEvent &e );
    bool buttonPressed( const OgreBites::ButtonEvent &e );
    bool buttonReleased( const OgreBites::ButtonEvent &e );

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
		*mCameraGoal,
		*mZoomNode;
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