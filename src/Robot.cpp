#include "Robot.h"
#include "ConfigScript.h"
#include "TUMBU.h"
//-------------------------------------------------------------------------------------
unsigned int Robot::instances = 0;
//-------------------------------------------------------------------
Robot::Robot(void){
	Robot::instances++;
	eyeGlowBoost = 0;

	ConfigNode* cfg;
	cfg = ConfigScriptLoader::getSingleton().getConfigScript( "game", "robot" );

	WALK_SPEED = cfg->findChild("walkSpeed")->getValueF(0);
	RUN_SPEED = cfg->findChild("runSpeed")->getValueF(0);
	WALK_ANIMATION_RATE = cfg->findChild("walkAnimationRate")->getValueF(0);
	RUN_ANIMATION_RATE = cfg->findChild("runAnimationRate")->getValueF(0);
	TURN_SPEED = cfg->findChild("turnSpeed")->getValueF(0);

	CHAR_RESTITUTION = cfg->findChild("restitution")->getValueF(0);
	CHAR_FRICTION = cfg->findChild("friction")->getValueF(0);
	CHAR_MASS = cfg->findChild("mass")->getValueF(0);

	HP_REGENERATION_PER_SECOND = cfg->findChild("hpRegenerationPerSecond")->getValueF(0);
	AP_REGENERATION_PER_SECOND = cfg->findChild("apRegenerationPerSecond")->getValueF(0);

	cfg = ConfigScriptLoader::getSingleton().getConfigScript( "skill", "punch" );
	PUNCH_DAMAGE = cfg->findChild("damage")->getValueI(0);

	cfg = ConfigScriptLoader::getSingleton().getConfigScript( "skill", "kick" );
	KICK_DAMAGE = cfg->findChild("damage")->getValueI(0);

	cfg = ConfigScriptLoader::getSingleton().getConfigScript( "skill", "jyn" );
	JYN_DAMAGE = cfg->findChild("damage")->getValueI(0);

	mKeyDirection	= Ogre::Vector3::ZERO;
	mGoalDirection	= Ogre::Vector3::ZERO;

	charBuilded = false;
	partsBuilded = false;

	isAttacking = false;
	isGuard = false;
	isRunning = false;

	statsRegenartionCount = 0;
	hpPercent = 1;
	apPercent = 1;

	specialManager = NULL;

	punch = NULL;
	kick = NULL;
	jyn = NULL;

	charRigidBody = NULL;
	charShape = NULL;

	robotNode = NULL;
	robotPhysicsNode = NULL;
}
//-------------------------------------------------------------------
Robot::~Robot(void){
	for(unsigned int i = 0; i < headList.size(); i++){
		if( headList[i] != NULL ){
			delete headList[i];
		}
    }

	for(unsigned int i = 0; i < bodyList.size(); i++){
		if (bodyList[i] != NULL ){
			delete bodyList[i];
		}
    }

	for(unsigned int i = 0; i < rightArmList.size(); i++){
		if( rightArmList[i] != NULL ){
			delete rightArmList[i];
		}
    }

	for(unsigned int i = 0; i < leftArmList.size(); i++){
		if( leftArmList[i] != NULL ){
			delete leftArmList[i];
		}
    }

	for(unsigned int i = 0; i < legsList.size(); i++){
		if( legsList[i] != NULL ){
			delete legsList[i];
		}
    }

	skillList.clear();

	if( punch != NULL ){
		delete punch;
	}
	if( kick != NULL ){
		delete kick;
	}
	if( jyn != NULL ){
		delete jyn;
	}

	headList.clear();
	bodyList.clear();
	rightArmList.clear();
	leftArmList.clear();
	legsList.clear();
	enemyList.clear();

	// The robot's nodes (parts, sound, specials are already gone). Each defeated enemy used to leave its
	// nodes in the scene until the match ended.
	Ogre::SceneManager *sceneMgr = TUMBU::getInstance()->mSceneMgr;
	if( robotNode != NULL ){
		SoundManager::getInstance()->destroySoundsUnder( robotNode );	// punch/kick sounds still pending
		robotNode->removeAndDestroyAllChildren();
		sceneMgr->destroySceneNode( robotNode );
	}
	if( robotPhysicsNode != NULL ){
		robotPhysicsNode->removeAndDestroyAllChildren();
		sceneMgr->destroySceneNode( robotPhysicsNode );
	}
}
//-------------------------------------------------------------------------------------
void Robot::updateEyeGlow( const Ogre::Real time ){
	bool charging = jyn != NULL && jyn->isAttacking() && jyn->special != NULL
		&& ( jyn->special->getSpecialStatus() == SpecialInterface::NONE			// cast: energy gathering
			|| jyn->special->getSpecialStatus() == SpecialInterface::CONCENTRATING
			|| jyn->special->getSpecialStatus() == SpecialInterface::CONCENTRATED );
	// Jyn takes three presses: cast (NONE), concentrate, attack. The eyes flare from the cast until the attack.
	// About a quarter of a second to flare up, half a second to calm down.
	Ogre::Real target = charging ? 1.0f : 0.0f;
	Ogre::Real rate = charging ? 4.0f : 2.0f;
	eyeGlowBoost += ( target - eyeGlowBoost ) * std::min( 1.0f, rate * time );

	if( head != NULL && head->entity != NULL ){
		for( unsigned int i = 0; i < head->entity->getNumSubEntities(); i++ ){
			head->entity->getSubEntity( i )->setCustomParameter( 0, Ogre::Vector4( eyeGlowBoost, 0, 0, 0 ) );
		}
	}
}
//-------------------------------------------------------------------
void Robot::setHorizontalVelocity( const Ogre::Vector3 &direction, Ogre::Real speed ){
	btRigidBody* rigid = charRigidBody->getBulletRigidBody();
	syncBodyRotation();
	rigid->activate( true );
	btVector3 velocity = rigid->getLinearVelocity();
	rigid->setLinearVelocity( btVector3( direction.x * speed, velocity.y(), direction.z * speed ) );
}
//-------------------------------------------------------------------
Ogre::Real Robot::getHorizontalSpeed(void) const{
	if( charRigidBody == NULL || charRigidBody->getBulletRigidBody() == NULL ){
		return 0;
	}
	btVector3 velocity = charRigidBody->getBulletRigidBody()->getLinearVelocity();
	return Ogre::Math::Sqrt( velocity.x() * velocity.x() + velocity.z() * velocity.z() );
}
//-------------------------------------------------------------------
Ogre::Real Robot::getLegsAnimationRate( bool running ) const{
	// The configured rate at the configured speed, scaled by the real ground speed (0 when stuck).
	Ogre::Real speed = running ? RUN_SPEED : WALK_SPEED;
	Ogre::Real rate = running ? RUN_ANIMATION_RATE : WALK_ANIMATION_RATE;
	if( speed <= 0 ){
		return rate;
	}
	return rate * std::min<Ogre::Real>( getHorizontalSpeed() / speed, 1.5f );
}
//-------------------------------------------------------------------
void Robot::syncBodyRotation(void){
	btRigidBody* rigid = charRigidBody->getBulletRigidBody();
	btQuaternion rotation = Physics::OgreBtConverter::to( robotNode->getOrientation() );

	// Only the rotation is copied: the physics simulation owns the position.
	btTransform transform = rigid->getWorldTransform();
	transform.setRotation( rotation );
	rigid->setWorldTransform( transform );

	transform = rigid->getInterpolationWorldTransform();
	transform.setRotation( rotation );
	rigid->setInterpolationWorldTransform( transform );
}
//-------------------------------------------------------------------
