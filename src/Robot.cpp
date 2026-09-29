#include "Robot.h"
#include "ConfigScript.h"
#include "TUMBU.h"
//-------------------------------------------------------------------------------------
unsigned int Robot::instances = 0;
//-------------------------------------------------------------------
Robot::Robot(void){
	Robot::instances++;

	ConfigNode* cfg;
	cfg = ConfigScriptLoader::getSingleton().getConfigScript( "game", "robot" );

	WALK_SPEED = cfg->findChild("walkSpeed")->getValueF(0);
	RUN_SPEED = cfg->findChild("runSpeed")->getValueF(0);
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

	/*
	robotNode->removeAndDestroyAllChildren();
	TUMBU::getInstance()->mSceneMgr->destroySceneNode( robotNode );

	cout << "FIM Classe pai" << endl;
	SoundManager::getInstance()->printAllSounds();
	*/
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
void Robot::syncBodyRotation(void){
	btRigidBody* rigid = charRigidBody->getBulletRigidBody();
	btQuaternion rotation = OgreBulletCollisions::OgreBtConverter::to( robotNode->getOrientation() );

	// Only the rotation is copied: the physics simulation owns the position.
	btTransform transform = rigid->getWorldTransform();
	transform.setRotation( rotation );
	rigid->setWorldTransform( transform );

	transform = rigid->getInterpolationWorldTransform();
	transform.setRotation( rotation );
	rigid->setInterpolationWorldTransform( transform );
}
//-------------------------------------------------------------------
