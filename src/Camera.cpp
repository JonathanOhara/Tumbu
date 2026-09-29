#include "Camera.h"
#include "TUMBU.h"
#include "GUI.h"
#ifdef TUMBU_DEBUG
#define MIN_CAMERA_DISTANCE 0
#define MAX_CAMERA_DISTANCE 100
#else
#define MIN_CAMERA_DISTANCE 2
#define MAX_CAMERA_DISTANCE 5
#endif
//-------------------------------------------------------------------------------------
Camera::Camera( Ogre::Camera* camera, Ogre::SceneNode* mChaseNode){
	mCamera = camera;
	chaseNode = mChaseNode;

	mCameraNode = camera->getSceneManager()->getRootSceneNode()->createChildSceneNode("camera_node");

	// create a pivot at roughly the character's shoulder
	mCameraPivot = camera->getSceneManager()->getRootSceneNode()->createChildSceneNode("camera_pivot_node");
	// this is where the camera should be soon, and it spins around the pivot
	mCameraGoal = mCameraPivot->createChildSceneNode( "camera_goal_node" );

	initiateCameraPosition();

	// The camera hangs from a zoom node under the chase node (Ogre 14 cameras are positioned by their node).
	if( camera->isAttached() ){
		camera->detachFromParent();
	}
	mZoomNode = mCameraNode->createChildSceneNode( "camera_zoom_node" );
	// In 2011 the camera kept its own local offset (0, 2, 4) under this node; the zoom range (2..5) is built on it.
	mZoomNode->setPosition( 0, 2, 4 );
	mZoomNode->attachObject(camera);

	screenFull			= GUI::getInstance()->getScreenWidth();
	
	animation = NONE;

	animatedByMouse = false;

	rotationFactor = 0;
	counter = 0;
}
//-------------------------------------------------------------------------------------
Camera::~Camera(void){
	mCameraNode->removeAndDestroyAllChildren();
	mCamera->getSceneManager()->destroySceneNode(mCameraNode);
	mCameraPivot->removeAndDestroyAllChildren();	// with the goal node
	mCamera->getSceneManager()->destroySceneNode(mCameraPivot);
}
//-------------------------------------------------------------------------------------
void Camera::initiateCameraPosition(void){
	ConfigNode *cfg;
	cfg = ConfigScriptLoader::getSingleton().getConfigScript("camera", "configuration");

	cameraZDistance = cfg->findChild("cameraZDistance")->getValueF();
	cameraHeight = cfg->findChild("cameraHeight")->getValueF();
	cameraTranslate = cfg->findChild("cameraTranslate")->getValueF();
	maxBoundPitch = cfg->findChild("maxBoundPitch")->getValueF();
	minBoundPitch = cfg->findChild("minBoundPitch")->getValueF();
	maxBoundZoom = cfg->findChild("maxBoundZoom")->getValueF();
	minBoundZoom = cfg->findChild("minBoundZoom")->getValueF();

	/*
	cout << "cameraZDistance " << cameraZDistance << endl;
	cout << "cameraHeight " << cameraHeight << endl;
	cout << "cameraTranslate " << cameraTranslate << endl;
	cout << "maxBoundPitch " << maxBoundPitch << endl;
	cout << "minBoundPitch " << minBoundPitch << endl;
	cout << "maxBoundZoom " << maxBoundZoom << endl;
	cout << "minBoundZoom " << minBoundZoom << endl;
	*/

	mCameraGoal->setPosition( 0, 0, cameraZDistance );

	mCameraPivot->setFixedYawAxis(true);
	mCameraGoal->setFixedYawAxis(true);
	mCameraNode->setFixedYawAxis(true);
	
	mCameraNode->setPosition(mCameraPivot->getPosition() + mCameraGoal->getPosition());

	mCameraNode->setAutoTracking(true, mCameraPivot);

	mPivotPitch = 0;
}
//-------------------------------------------------------------------------------------
bool Camera::keyPressed( const OgreBites::KeyboardEvent &input){
	if( TUMBU::getInstance()->isPlaying() ){
		switch( input.keysym.sym ) {
		case 'q':
			animation = ROTATING_RIGHT;
			rotationFactor = 1.3f;
			break;
		case 'e':
			animation = ROTATING_LEFT;
			rotationFactor = 1.3f;
			break;
		case TumbuInput::KEY_PAGEUP:
			animation = ZOOM_IN;
			break;
		case TumbuInput::KEY_PAGEDOWN:
			animation = ZOOM_OUT;
			break;
		case 'c':
			initiateCameraPosition();
			break;                
	#ifdef TUMBU_DEBUG
		case TumbuInput::KEY_HOME:
			animation = ROTATING_UP;
			break;
		case TumbuInput::KEY_END:
			animation = ROTATING_DOWN;
			break;
	#endif	
		}
	}

	return true;
}
//-------------------------------------------------------------------------------------
bool Camera::keyReleased( const OgreBites::KeyboardEvent &input){
	if( TUMBU::getInstance()->isPlaying() ){
		switch( input.keysym.sym ) {
		case 'q':
			if( animation == ROTATING_RIGHT ){
				animation = NONE;
			}
			break;
		case 'e':
			if( animation == ROTATING_LEFT ){
				animation = NONE;
			}
			break;
		case TumbuInput::KEY_PAGEUP:
			if( animation == ZOOM_IN ){
				animation = NONE;
			}
			break;
		case TumbuInput::KEY_PAGEDOWN:
			if( animation == ZOOM_OUT ){
				animation = NONE;
			}
			break;
	#ifdef TUMBU_DEBUG
		case TumbuInput::KEY_HOME:
			if( animation == ROTATING_UP ){
				animation = NONE;
			}
			break;
		case TumbuInput::KEY_END:
			if( animation == ROTATING_DOWN ){
				animation = NONE;
			}
			break;
	#endif
		}
	}

	return true;
}
//-------------------------------------------------------------------------------------
bool Camera::hatMoved( const OgreBites::HatEvent &e ) { 
	return true;
}
//-------------------------------------------------------------------------------------
bool Camera::axisMoved( const OgreBites::AxisEvent &e ) {
	if( !TUMBU::getInstance()->isPlaying() ){
		return true;
	}

	// Triggers rotate the camera (the 2011 pad used buttons 6/7), and so does the right stick.
	switch( e.axis ){
	case TumbuInput::AXIS_TRIGGERLEFT:
	case TumbuInput::AXIS_TRIGGERRIGHT:
	case TumbuInput::AXIS_RIGHTX:{
		bool left = e.axis == TumbuInput::AXIS_TRIGGERLEFT || ( e.axis == TumbuInput::AXIS_RIGHTX && e.value < 0 );
		CameraAnimation wanted = left ? ROTATING_LEFT : ROTATING_RIGHT;
		if( Ogre::Math::Abs( e.value ) > TumbuInput::AXIS_DEADZONE ){
			animation = wanted;
			rotationFactor = 1.5f;
		}else if( animation == ROTATING_LEFT || animation == ROTATING_RIGHT ){
			animation = NONE;
		}
		break;
	}
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool Camera::buttonPressed( const OgreBites::ButtonEvent &e ) {
	return true;
}
//-------------------------------------------------------------------------------------
bool Camera::buttonReleased( const OgreBites::ButtonEvent &e ) {
	return true;
}
//-------------------------------------------------------------------------------------
bool Camera::mouseMoved( const OgreBites::MouseMotionEvent &arg ){
	if( TUMBU::getInstance()->isPlaying() ){
		updateCameraGoal( -0.05f * arg.xrel, -0.05f * arg.yrel, 0 );
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool Camera::mouseWheelRolled( const OgreBites::MouseWheelEvent &arg ){
	if( TUMBU::getInstance()->isPlaying() ){
		// OIS reported 120 per wheel notch; SDL reports 1.
		updateCameraGoal( 0, 0, -0.06f * arg.y );
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool Camera::mousePressed( const OgreBites::MouseButtonEvent &arg ){
	return true;
}
//-------------------------------------------------------------------------------------
bool Camera::mouseReleased( const OgreBites::MouseButtonEvent &arg ){
	return true;
}
//-------------------------------------------------------------------------------------
bool Camera::frameRenderingQueued(const Ogre::FrameEvent &evt){
	/*
	counter++;

	if( counter != 5 ){
		returatriton true;
	}

	counter = 0;
	*/
	if( TUMBU::getInstance()->isPlaying() ){
		Ogre::Vector3 chasePosition = chaseNode->getPosition();

		// place the camera pivot roughly at the character's shoulder
		mCameraPivot->setPosition(chasePosition + Ogre::Vector3::UNIT_Y * cameraHeight);
		// move the camera smoothly to the goal
		Ogre::Vector3 goalOffset = mCameraGoal->_getDerivedPosition() - mCameraNode->getPosition();
		// Clamp the catch-up factor so a long frame cannot overshoot the goal.
		mCameraNode->translate(goalOffset * std::min<Ogre::Real>(1.0f, evt.timeSinceLastFrame * cameraTranslate));

		// always look at the pivot		
		
/*
		mCameraNode->lookAt(mCameraPivot->_getDerivedPosition(), Ogre::Node::TS_WORLD);
		Ogre::Vector3 orient = mCameraPivot->_getDerivedPosition() - mCameraNode->_getDerivedPosition();

		mCameraNode->setDirection(orient, Ogre::Node::TS_WORLD);
*/
		
		switch( animation){
		case ROTATING_RIGHT:
			mCameraPivot->yaw(Ogre::Radian(evt.timeSinceLastFrame * rotationFactor));
			break;
		case ROTATING_LEFT:
			mCameraPivot->yaw(Ogre::Radian(evt.timeSinceLastFrame * - rotationFactor));
			break;
		case ZOOM_IN:
			if( mZoomNode->getPosition().z > MIN_CAMERA_DISTANCE){
				mZoomNode->setPosition(mZoomNode->getPosition() + Ogre::Vector3(0, 0, -10 * evt.timeSinceLastFrame));
			}
			break;
		case ZOOM_OUT:
			if( mZoomNode->getPosition().z < MAX_CAMERA_DISTANCE){
				mZoomNode->setPosition(mZoomNode->getPosition() + Ogre::Vector3(0, 0, 10 * evt.timeSinceLastFrame));
			}
			break;
	#ifdef TUMBU_DEBUG
		case ROTATING_UP:
			mCameraPivot->pitch(Ogre::Radian(evt.timeSinceLastFrame * - 2));
			break;
		case ROTATING_DOWN:
			mCameraPivot->pitch(Ogre::Radian(evt.timeSinceLastFrame * 2));
			break;
	#endif
		}
	}

	return true;
}
//-------------------------------------------------------------------------------------
void Camera::updateCameraGoal(Ogre::Real deltaYaw, Ogre::Real deltaPitch, Ogre::Real deltaZoom){
//	cout << "Yaw " << deltaYaw << endl;
	mCameraPivot->yaw(Ogre::Degree(deltaYaw), Ogre::Node::TS_WORLD);

	// bound the pitch
	if (!(mPivotPitch + deltaPitch > maxBoundPitch && deltaPitch > 0) &&
		!(mPivotPitch + deltaPitch < minBoundPitch && deltaPitch < 0)) {
		mCameraPivot->pitch(Ogre::Degree(deltaPitch), Ogre::Node::TS_LOCAL);
		mPivotPitch += deltaPitch;
	}
		
	Ogre::Real dist = mCameraGoal->_getDerivedPosition().distance(mCameraPivot->_getDerivedPosition());
	Ogre::Real distChange = deltaZoom * dist;

//	cout << "dist = " << dist << " change = " << distChange << " sum = " << ( dist + distChange ) << endl;

	// bound the zoom
	if (!(dist + distChange < minBoundZoom && distChange < 0) &&
		!(dist + distChange > maxBoundZoom && distChange > 0)) {
//		cout << "update camera goal " << endl;
		mCameraGoal->translate(0, 0, distChange, Ogre::Node::TS_LOCAL);
	}
}
//-------------------------------------------------------------------------------------
void Camera::startExplorationMode(){
}
//-------------------------------------------------------------------------------------
void Camera::startFightMode(Ogre::SceneNode* mLockNode){
	lockNode = mLockNode;
}
//-------------------------------------------------------------------------------------
Camera::CameraAnimation Camera::getAnimation(){
	return animation;
}
//-------------------------------------------------------------------------------------
