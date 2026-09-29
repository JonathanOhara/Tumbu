#include "SimpleRigidBody.h"
#include "TUMBU.h"
//-------------------------------------------------------------------------------------
SimpleRigidBody::SimpleRigidBody( Ogre::SceneManager *_sceneManager, TumbuEnums::PhysicObjectTag _objectTag ){
	rigidBody		= NULL;
	shape			= NULL;
	node			= NULL;
	entity			= NULL;

	sceneManager = _sceneManager;
	objectTag = _objectTag;
}
//-------------------------------------------------------------------------------------
SimpleRigidBody::~SimpleRigidBody(void){
	if(rigidBody){
		TUMBU::getInstance()->removeCollisionDetectionListener( this );
		delete rigidBody;	// also frees the Bullet body and motion state
	}

	if(shape){
		delete shape;
	}

	if(entity){
		sceneManager->destroyEntity( entity );
	}

	if(node){
		node->removeAndDestroyAllChildren();
		sceneManager->destroySceneNode( node );
	}
}
//-------------------------------------------------------------------------------------
void SimpleRigidBody::collision( CollisionDetectionListener *other ){
}
//-------------------------------------------------------------------------------------
Physics::RigidBody* SimpleRigidBody::getOgreBulletRigidBody( const std::string& instanceName ){
	return rigidBody;
}
//-------------------------------------------------------------------------------------
void SimpleRigidBody::setOgreBulletRigidBody( Physics::RigidBody *_rigidBody ){
	rigidBody = _rigidBody;
	TUMBU::getInstance()->addCollisionDetectionListener( this );
}
//-------------------------------------------------------------------------------------