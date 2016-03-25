#include "SpecialKick.h"
#include "Robot.h"
#include "GUI.h"
//-------------------------------------------------------------------------------------
SpecialKick::SpecialKick( Ogre::SceneManager* _sceneMgr, Ogre::SceneNode* _particleSystemNode, Robot* _speller, OgreBulletDynamics::DynamicsWorld* _world, int _count, float _damage ){
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
	particleSystem->setMaterialName(PARTICLE_MATERIAL);
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
			particleList[0]->particle->position = specialRigidBodyList.front()->getWorldPosition();
		}

		if(timeToResest <= 0){
			setSpecialStatus( SpecialInterface::TO_DELETE );
		}
	}
}
//-------------------------------------------------------------------------------------
void SpecialKick::collision( CollisionDetectionListener *other ){
	if( getSpecialStatus() == SpecialInterface::ATTACKING ){
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
OgreBulletDynamics::RigidBody* SpecialKick::getOgreBulletRigidBody( const std::string& instanceName ){
	OgreBulletDynamics::RigidBody 
		*returnObject = NULL,
		*specialRigid;

	std::list<OgreBulletDynamics::RigidBody*>::iterator i = specialRigidBodyList.begin();
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
			
	particula->particle->setDimensions( PARTICLE_WIDTH * 2, PARTICLE_HEIGHT * 2 );
	particula->particle->timeToLive = PARTICLE_LIVE_TIME;
	particula->particle->colour = PARTICLE_COLOR;
	particula->particle->direction = Ogre::Vector3::ZERO;
	particula->particle->rotationSpeed = 0;
	particula->particle->position = Ogre::Vector3(position);

	particleList.push_back(particula);
}
//-------------------------------------------------------------------------------------
void SpecialKick::attack(Ogre::Quaternion orientation){
	setSpecialStatus( SpecialInterface::ATTACKING );
	
	OgreBulletDynamics::RigidBody* specialRigidBody;
	
	Ogre::Vector3 rigidBodyPosition(robotSpeller->legsNode->_getDerivedPosition() );
	rigidBodyPosition += robotSpeller->robotNode->getOrientation() * Ogre::Vector3(0, 0, 0.5f);

	Ogre::String nodeName = robotSpeller->robotName + "_punch_node_" + Ogre::StringConverter::toString(count);
	Ogre::String rightBodyName = robotSpeller->robotName + "_punch_rigidbody_" + Ogre::StringConverter::toString(count);

	specialRigidNode = particleSystemNode->getParentSceneNode()->getParentSceneNode()->createChildSceneNode( nodeName );

	specialShape = new OgreBulletCollisions::BoxCollisionShape( Ogre::Vector3( PARTICLE_WIDTH, PARTICLE_WIDTH, PARTICLE_WIDTH ) );
	specialRigidBody = new OgreBulletDynamics::RigidBody( rightBodyName, world );

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

	specialRigidBody->applyImpulse( 
		translation, Ogre::Vector3(0, 0, 0) );

	specialRigidBodyList.push_back( specialRigidBody );

	TUMBU::getInstance()->addCollisionDetectionListener( this, specialRigidBody->getName() );

	timeToResest = 3;
}
//-------------------------------------------------------------------------------------
void SpecialKick::hit(Ogre::SceneNode* hittedNode){
	setSpecialStatus( SpecialInterface::HITTED );
}
//-------------------------------------------------------------------------------------
void SpecialKick::hitScenario( Ogre::Vector3 position ){
	setSpecialStatus( SpecialInterface::HITTED );
}
//-------------------------------------------------------------------------------------
void SpecialKick::toDelete(){
	setSpecialStatus( SpecialInterface::TO_DELETE );
}
//-------------------------------------------------------------------------------------
void SpecialKick::clear(){
	for(unsigned int i = 0; i < particleList.size(); i++){
		delete particleList[i];
    }
    
	particleList.clear();
    particleSystem->clear();

	std::list<OgreBulletDynamics::RigidBody*>::iterator i = specialRigidBodyList.begin();
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