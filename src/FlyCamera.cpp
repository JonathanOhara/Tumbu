#include "FlyCamera.h"
#include "ConfigScript.h"
#include "GUI.h"
#include "Input.h"
#include "TUMBU.h"
//-------------------------------------------------------------------------------------
static ConfigNode* cameraConfig( const char* key ){
	ConfigNode* cfg = ConfigScriptLoader::getSingleton().getConfigScript( "camera", "configuration" );
	return cfg != NULL ? cfg->findChild( key ) : NULL;
}
//-------------------------------------------------------------------------------------
FlyCamera::FlyCamera( Ogre::Camera* _camera ){
	camera = _camera;
	flying = false;
	stopRequested = false;
	moveForward = moveBack = moveLeft = moveRight = moveUp = moveDown = fast = false;
	lookLeft = lookRight = lookUp = lookDown = false;
	ConfigNode* speedNode = cameraConfig( "flySpeed" );
	speed = speedNode != NULL ? speedNode->getValueF() : 8.0f;
}
//-------------------------------------------------------------------------------------
FlyCamera::~FlyCamera(void){
	// Nothing to restore: the match (and the chase camera's nodes) is being destroyed.
}
//-------------------------------------------------------------------------------------
bool FlyCamera::isEnabled(void){
	ConfigNode* node = cameraConfig( "flyCamera" );
	return node == NULL || node->getValueI() != 0;
}
//-------------------------------------------------------------------------------------
void FlyCamera::start(void){
	TUMBU* tumbu = TUMBU::getInstance();
	Ogre::SceneNode* node = camera->getParentSceneNode();
	savedPosition = node->getPosition();
	savedOrientation = node->getOrientation();

	// Start from the current view.
	position = camera->getDerivedPosition();
	Ogre::Vector3 direction = camera->getDerivedDirection();
	yaw = Ogre::Math::ATan2( -direction.x, -direction.z );
	pitch = Ogre::Math::ASin( Ogre::Math::Clamp<Ogre::Real>( direction.y, -1, 1 ) );

	moveForward = moveBack = moveLeft = moveRight = moveUp = moveDown = fast = false;
	lookLeft = lookRight = lookUp = lookDown = false;
	flying = true;
	tumbu->setGameState( TumbuEnums::FLYING );
	tumbu->setMouseCaptured( true );
	GUI::getInstance()->addLog( Log::GREEN, "Fly: WASD, mouse, Space/C up/down, Shift fast, F exits", 6 );
	Ogre::LogManager::getSingleton().logMessage( "FlyCamera: flying" );
}
//-------------------------------------------------------------------------------------
void FlyCamera::stop(void){
	Ogre::SceneNode* node = camera->getParentSceneNode();
	node->setPosition( savedPosition );
	node->setOrientation( savedOrientation );
	flying = false;
	TUMBU::getInstance()->setGameState( TumbuEnums::PLAYING );
	Ogre::LogManager::getSingleton().logMessage( "FlyCamera: back to the match" );
}
//-------------------------------------------------------------------------------------
void FlyCamera::applyOrientation(void){
	Ogre::SceneNode* node = camera->getParentSceneNode();
	Ogre::Quaternion orientation = Ogre::Quaternion( yaw, Ogre::Vector3::UNIT_Y ) * Ogre::Quaternion( pitch, Ogre::Vector3::UNIT_X );
	// The camera hangs below the chase camera's nodes: set world values so the parents do not matter.
	node->_setDerivedPosition( position );
	node->_setDerivedOrientation( orientation );
}
//-------------------------------------------------------------------------------------
bool FlyCamera::frameRenderingQueued( const Ogre::FrameEvent &evt ){
	if( !flying ){
		return true;
	}
	if( stopRequested ){
		stopRequested = false;
		stop();
		return true;
	}
	Ogre::Real t = evt.timeSinceLastFrame;
	const Ogre::Real lookSpeed = 1.5f;	// radians per second (arrow keys)
	if( lookLeft )	yaw += Ogre::Radian( lookSpeed * t );
	if( lookRight )	yaw -= Ogre::Radian( lookSpeed * t );
	if( lookUp )	pitch += Ogre::Radian( lookSpeed * t );
	if( lookDown )	pitch -= Ogre::Radian( lookSpeed * t );
	pitch = Ogre::Radian( Ogre::Math::Clamp<Ogre::Real>( pitch.valueRadians(), -1.5f, 1.5f ) );

	Ogre::Quaternion orientation = Ogre::Quaternion( yaw, Ogre::Vector3::UNIT_Y ) * Ogre::Quaternion( pitch, Ogre::Vector3::UNIT_X );
	Ogre::Vector3 move = Ogre::Vector3::ZERO;
	if( moveForward )	move += orientation * Ogre::Vector3::NEGATIVE_UNIT_Z;
	if( moveBack )		move += orientation * Ogre::Vector3::UNIT_Z;
	if( moveLeft )		move += orientation * Ogre::Vector3::NEGATIVE_UNIT_X;
	if( moveRight )		move += orientation * Ogre::Vector3::UNIT_X;
	if( moveUp )		move += Ogre::Vector3::UNIT_Y;
	if( moveDown )		move += Ogre::Vector3::NEGATIVE_UNIT_Y;
	if( move != Ogre::Vector3::ZERO ){
		position += move.normalisedCopy() * speed * ( fast ? 4.0f : 1.0f ) * t;
	}
	applyOrientation();
	return true;
}
//-------------------------------------------------------------------------------------
bool FlyCamera::keyPressed( const OgreBites::KeyboardEvent &evt ){
	TUMBU* tumbu = TUMBU::getInstance();
	int key = evt.keysym.sym;
	if( !flying ){
		if( key == 'f' && tumbu->isPlaying() ){
			start();
		}
		return true;
	}
	switch( key ){
	case 'f':
	case TumbuInput::KEY_ESCAPE:	stopRequested = true; break;	// next frame: the GUI must still see FLYING for this Esc
	case 'w':						moveForward = true; break;
	case 's':						moveBack = true; break;
	case 'a':						moveLeft = true; break;
	case 'd':						moveRight = true; break;
	case TumbuInput::KEY_SPACE:		moveUp = true; break;
	case 'c':						moveDown = true; break;
	case TumbuInput::KEY_LSHIFT:	fast = true; break;
	case OgreBites::SDLK_LEFT:		lookLeft = true; break;
	case OgreBites::SDLK_RIGHT:		lookRight = true; break;
	case OgreBites::SDLK_UP:		lookUp = true; break;
	case OgreBites::SDLK_DOWN:		lookDown = true; break;
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool FlyCamera::keyReleased( const OgreBites::KeyboardEvent &evt ){
	switch( evt.keysym.sym ){
	case 'w':						moveForward = false; break;
	case 's':						moveBack = false; break;
	case 'a':						moveLeft = false; break;
	case 'd':						moveRight = false; break;
	case TumbuInput::KEY_SPACE:		moveUp = false; break;
	case 'c':						moveDown = false; break;
	case TumbuInput::KEY_LSHIFT:	fast = false; break;
	case OgreBites::SDLK_LEFT:		lookLeft = false; break;
	case OgreBites::SDLK_RIGHT:		lookRight = false; break;
	case OgreBites::SDLK_UP:		lookUp = false; break;
	case OgreBites::SDLK_DOWN:		lookDown = false; break;
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool FlyCamera::mouseMoved( const OgreBites::MouseMotionEvent &evt ){
	if( flying ){
		const Ogre::Real sensitivity = 0.003f;	// radians per pixel
		yaw -= Ogre::Radian( evt.xrel * sensitivity );
		pitch -= Ogre::Radian( evt.yrel * sensitivity );
	}
	return true;
}
