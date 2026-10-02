#include "SpecialKick.h"
#include "TUMBU.h"
#include "Robot.h"
#include "GUI.h"
#include "EffectsManager.h"
#include <OgreParticleSystemRenderer.h>
//-------------------------------------------------------------------------------------
SpecialKick::SpecialKick( Ogre::SceneManager* _sceneMgr, Ogre::SceneNode* _particleSystemNode, Robot* _speller, Physics::DynamicsWorld* _world, int _count, float _damage ){
	world = _world;
	sceneMgr = _sceneMgr;
	particleSystemNode = _particleSystemNode;
	robotSpeller = _speller;
	count = _count;
	damage = _damage;

	setSpecialStatus( SpecialInterface::NONE );

	particleSystem = sceneMgr->createParticleSystem();
	particleSystemNode->attachObject( particleSystem );	

	particleSystem->_update(1);
	particleSystem->setDefaultDimensions( PARTICLE_WIDTH, PARTICLE_HEIGHT );
	particleSystem->setMaterialName(CRESCENT_MATERIAL);
	// The wave lies along its flight (mDirection), its curve forward.
	particleSystem->getRenderer()->setParameter( "billboard_type", "oriented_self" );
	particleSystem->setSpeedFactor(0);

	specialRigidNode = NULL;
	specialShape = NULL;

	concentrate();
}
//-------------------------------------------------------------------------------------
SpecialKick::~SpecialKick(void){
	clear();

	sceneMgr->destroyParticleSystem( particleSystem );
	
	setSpecialStatus( SpecialInterface::FINISHED );
}
//-------------------------------------------------------------------------------------
void SpecialKick::update(const Ogre::Real time){
	if( getSpecialStatus() == SpecialInterface::ATTACKING || getSpecialStatus() == SpecialInterface::HITTED){
		timeToResest -= time;
			
		if( specialShape != NULL ){
			particleList[0]->particle->mPosition = specialRigidBodyList.front()->getWorldPosition();
			updateBallEffect( particleList[0]->particle->mPosition );
		}
		if( getSpecialStatus() == SpecialInterface::HITTED && particleList[0]->active ){
			// The impact effect took over: the ball is gone (its body flies on, harmless, until the timeout).
			particleList[0]->active = false;
			particleList[0]->particle->setDimensions( 0, 0 );
			releaseBallEffect();
		}

		if(timeToResest <= 0){
			setSpecialStatus( SpecialInterface::TO_DELETE );
		}
	}
}
//-------------------------------------------------------------------------------------
void SpecialKick::collision( CollisionDetectionListener *other ){
	if( getSpecialStatus() == SpecialInterface::ATTACKING ){
		if( ( other->objectTag == TumbuEnums::TERRAIN || other->objectTag == TumbuEnums::SCENE_OBJECT ) && timeToResest < 2.9f ){
			hitScenario( particleList[0]->particle->mPosition );
			return;
		}
		Robot *enemy;

		btRigidBody* otherRigidBody = other->getOgreBulletRigidBody( other->rigidBodyName )->getBulletRigidBody();

		std::list<Robot*>::iterator j;
		j = robotSpeller->enemyList.begin();
		while ( j != robotSpeller->enemyList.end() ){
			enemy = (*j);

			if( enemy->charRigidBody->getBulletRigidBody() == otherRigidBody ){
				enemy->emmitSound( "specialKickSound", "kick", "punch.ogg", false, true, 0.0f);
				hit( enemy->robotNode );
				float dano = enemy->sofrerDano( robotSpeller->criarDano(damage) );
				GUI::getInstance()->addSkillHit( enemy->robotNode->getPosition(), Ogre::StringConverter::toString( dano ) );
			}
			j++;
		}
	}
}
//-------------------------------------------------------------------------------------
Physics::RigidBody* SpecialKick::getOgreBulletRigidBody( const std::string& instanceName ){
	Physics::RigidBody 
		*returnObject = NULL,
		*specialRigid;

	std::list<Physics::RigidBody*>::iterator i = specialRigidBodyList.begin();
	while ( i != specialRigidBodyList.end() ){
		specialRigid = (*i);
		if( specialRigid->getName() == instanceName ){
			returnObject = specialRigid;
			break;
		}
		i++;
	}

	return returnObject;
}
//-------------------------------------------------------------------------------------
void SpecialKick::concentrate(){
	setSpecialStatus( SpecialInterface::CONCENTRATED );

	EnergyParticle* particula = new EnergyParticle( particleSystem->createParticle() );
	particula->active = true;

	Ogre::Vector3 position(	robotSpeller->legsNode->_getDerivedPosition() );
	position += robotSpeller->robotNode->getOrientation() * Ogre::Vector3(0, 0, 0.5f);
			
	particula->particle->setDimensions( CRESCENT_WIDTH, CRESCENT_HEIGHT );
	particula->particle->mTimeToLive = PARTICLE_LIVE_TIME;
	particula->particle->mColour = orbColour( getKiColour( "kick" ) );
	particula->particle->mDirection = robotSpeller->robotNode->getOrientation() * Ogre::Vector3::UNIT_Z;	// it faces where it will fly
	particula->particle->mRotationSpeed = 0;
	particula->particle->mPosition = Ogre::Vector3(position);

	particleList.push_back(particula);
	startBallEffect( "kick_ball", position, getKiColour( "kick" ) );
}
//-------------------------------------------------------------------------------------
void SpecialKick::attack(Ogre::Quaternion orientation){
	setSpecialStatus( SpecialInterface::ATTACKING );
	
	Physics::RigidBody* specialRigidBody;
	
	Ogre::Vector3 rigidBodyPosition(robotSpeller->legsNode->_getDerivedPosition() );
	rigidBodyPosition += robotSpeller->robotNode->getOrientation() * Ogre::Vector3(0, 0, 0.5f);

	Ogre::String nodeName = robotSpeller->robotName + "_punch_node_" + Ogre::StringConverter::toString(count);
	Ogre::String rightBodyName = robotSpeller->robotName + "_punch_rigidbody_" + Ogre::StringConverter::toString(count);

	specialRigidNode = particleSystemNode->getParentSceneNode()->getParentSceneNode()->createChildSceneNode( nodeName );

	// Half extents: the crescent wave's width across, most of its height, a thin depth along the flight.
	specialShape = new Physics::BoxCollisionShape( Ogre::Vector3( CRESCENT_WIDTH * 0.45f, CRESCENT_HEIGHT * 0.35f, PARTICLE_WIDTH ) );
	specialRigidBody = new Physics::RigidBody( rightBodyName, world );

	specialRigidBody->setShape( specialRigidNode, 
		specialShape,
		0.1f,         // dynamic body restitution
		1.0f,         // dynamic body friction
		10,          // dynamic bodymass
		rigidBodyPosition,
		orientation
	);// orientation of the box

	specialRigidBody->getBulletRigidBody()->setGravity( btVector3(0,0,0) );

	Ogre::Vector3 translation = orientation * Ogre::Vector3(0, 0, 200);
	if( !particleList.empty() ){
		particleList[0]->particle->mDirection = ( orientation * Ogre::Vector3::UNIT_Z ).normalisedCopy();
	}

	specialRigidBody->applyImpulse( 
		translation, Ogre::Vector3(0, 0, 0) );

	specialRigidBodyList.push_back( specialRigidBody );

	TUMBU::getInstance()->addCollisionDetectionListener( this, specialRigidBody->getName() );

	// Release flash where the blast leaves the robot.
	if( EffectsManager::getInstance() != NULL ){
		EffectsManager::getInstance()->spawn( "kick_muzzle", rigidBodyPosition, getKiColour( "kick" ) );
	}

	timeToResest = 3;
}
//-------------------------------------------------------------------------------------
void SpecialKick::hit(Ogre::SceneNode* hittedNode){
	if( EffectsManager::getInstance() != NULL && !particleList.empty() ){
		EffectsManager::getInstance()->spawn( "blast_hit", particleList[0]->particle->mPosition, getKiColour( "kick" ) );
	}
	setSpecialStatus( SpecialInterface::HITTED );
}
//-------------------------------------------------------------------------------------
void SpecialKick::hitScenario( Ogre::Vector3 position ){
	if( EffectsManager::getInstance() != NULL ){
		EffectsManager::getInstance()->spawn( "blast_wall", position, getKiColour( "kick" ) );
	}
	setSpecialStatus( SpecialInterface::HITTED );
}
//-------------------------------------------------------------------------------------
void SpecialKick::toDelete(){
	setSpecialStatus( SpecialInterface::TO_DELETE );
}
//-------------------------------------------------------------------------------------
void SpecialKick::clear(){
	releaseBallEffect();
	for(unsigned int i = 0; i < particleList.size(); i++){
		delete particleList[i];
    }
    
	particleList.clear();
    particleSystem->clear();

	std::list<Physics::RigidBody*>::iterator i = specialRigidBodyList.begin();
	while ( i != specialRigidBodyList.end() ){
		TUMBU::getInstance()->removeCollisionDetectionListener( (*i)->getName() );
		delete *i;
		i = specialRigidBodyList.erase(i);
	}
	specialRigidBodyList.clear();

    if( specialShape != NULL ){
		delete specialShape;
		specialShape = NULL;
	}

	if( specialRigidNode != NULL ){
		specialRigidNode->removeAndDestroyAllChildren();
		sceneMgr->destroySceneNode(specialRigidNode);
		specialRigidNode = NULL;
	}

	timeToResest = 0;
}
//-------------------------------------------------------------------------------------