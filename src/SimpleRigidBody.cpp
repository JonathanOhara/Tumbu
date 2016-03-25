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
	try{
		if(rigidBody){
			TUMBU::getInstance()->removeCollisionDetectionListener( this );

			/*
			delete rigidBody->getBulletRigidBody()->getMotionState();
			delete rigidBody->getBulletRigidBody();
			*/
			rigidBody->showDebugShape(false);
			delete rigidBody;
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
	}catch( char * str ) {
      cout << "Exception raised: " << str << '\n';
   }
}
//-------------------------------------------------------------------------------------
void SimpleRigidBody::collision( CollisionDetectionListener *other ){
}
//-------------------------------------------------------------------------------------
OgreBulletDynamics::RigidBody* SimpleRigidBody::getOgreBulletRigidBody( const std::string& instanceName ){
	return rigidBody;
}
//-------------------------------------------------------------------------------------
void SimpleRigidBody::setOgreBulletRigidBody( OgreBulletDynamics::RigidBody *_rigidBody ){
	rigidBody = _rigidBody;
	TUMBU::getInstance()->addCollisionDetectionListener( this );
}
//-------------------------------------------------------------------------------------