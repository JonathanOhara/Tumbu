#include "Camera.h"
#include "TUMBU.h"
#include "GUI.h"
#ifdef _DEBUG
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

	mCameraNode->attachObject(camera);

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
bool Camera::keyPressed( const OIS::KeyEvent& input){
	if( TUMBU::getInstance()->isPlaying() ){
		switch( input.key ) {
		case OIS::KC_Q:
			animation = ROTATING_RIGHT;
			rotationFactor = 1.3f;
			break;
		case OIS::KC_E:
			animation = ROTATING_LEFT;
			rotationFactor = 1.3f;
			break;
		case OIS::KC_PGUP:
			animation = ZOOM_IN;
			break;
		case OIS::KC_PGDOWN:
			animation = ZOOM_OUT;
			break;
		case OIS::KC_C:
			initiateCameraPosition();
			break;                
	#ifdef _DEBUG
		case OIS::KC_HOME:
			animation = ROTATING_UP;
			break;
		case OIS::KC_END:
			animation = ROTATING_DOWN;
			break;
	#endif	
		}
	}

	return true;
}
//-------------------------------------------------------------------------------------
bool Camera::keyReleased( const OIS::KeyEvent& input){
	if( TUMBU::getInstance()->isPlaying() ){
		switch( input.key ) {
		case OIS::KC_Q:
			if( animation == ROTATING_RIGHT ){
				animation = NONE;
			}
			break;
		case OIS::KC_E:
			if( animation == ROTATING_LEFT ){
				animation = NONE;
			}
			break;
		case OIS::KC_PGUP:
			if( animation == ZOOM_IN ){
				animation = NONE;
			}
			break;
		case OIS::KC_PGDOWN:
			if( animation == ZOOM_OUT ){
				animation = NONE;
			}
			break;
	#ifdef _DEBUG
		case OIS::KC_HOME:
			if( animation == ROTATING_UP ){
				animation = NONE;
			}
			break;
		case OIS::KC_END:
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
bool Camera::povMoved( const OIS::JoyStickEvent &e, int pov ) { 
	return true;
}
//-------------------------------------------------------------------------------------
bool Camera::axisMoved( const OIS::JoyStickEvent &e, int axis ) {
	return true;
}
//-------------------------------------------------------------------------------------
bool Camera::sliderMoved( const OIS::JoyStickEvent &e, int sliderID ) {
	return true;
}
//------------------------------------------------------------------------------------- 
bool Camera::buttonPressed( const OIS::JoyStickEvent &e, int button ) {
	switch( button ){
	case 6:
		animation = ROTATING_LEFT;
		break;
	case 7:
		animation = ROTATING_RIGHT;
		break;
	}

	return true;
}
//-------------------------------------------------------------------------------------
bool Camera::buttonReleased( const OIS::JoyStickEvent &e, int button ) {
	if( TUMBU::getInstance()->isPlaying() ){
		switch( button ){
		case 6:
			if( animation == ROTATING_LEFT ){
				animation = NONE;
				rotationFactor = 1.5f;
			}
			break;
		case 7:
			if( animation == ROTATING_RIGHT ){
				animation = NONE;
				rotationFactor = 1.5f;
			}
			break;
		}
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool Camera::mouseMoved( const OIS::MouseEvent &arg ){
	if( TUMBU::getInstance()->isPlaying() ){
		updateCameraGoal(-0.05f * arg.state.X.rel, -0.05f * arg.state.Y.rel, -0.0005f * arg.state.Z.rel);
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool Camera::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id ){
	return true;
}
//-------------------------------------------------------------------------------------
bool Camera::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id ){
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
		mCameraNode->translate(goalOffset * evt.timeSinceLastFrame * cameraTranslate);

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
			if( mCamera->getPosition().z > MIN_CAMERA_DISTANCE){
				mCamera->setPosition(mCamera->getPosition() + Ogre::Vector3(0, 0, -10 * evt.timeSinceLastFrame));
			}
			break;
		case ZOOM_OUT:
			if( mCamera->getPosition().z < MAX_CAMERA_DISTANCE){
				mCamera->setPosition(mCamera->getPosition() + Ogre::Vector3(0, 0, 10 * evt.timeSinceLastFrame));
			}
			break;
	#ifdef _DEBUG
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