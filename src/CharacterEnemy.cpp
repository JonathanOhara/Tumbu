#include "CharacterEnemy.h"
#include "Util.h"
#include "TUMBU.h"
//-------------------------------------------------------------------------------------
CharacterEnemy::CharacterEnemy(OgreBulletDynamics::DynamicsWorld* _physicWorld, Ogre::String _robotName, SoundManager *_soundManager){
	physicWorld = _physicWorld;
	robotName = _robotName.append( Ogre::StringConverter::toString( Robot::instances ) );
	soundManager = _soundManager;

	ConfigNode* cfg;
	cfg = ConfigScriptLoader::getSingleton().getConfigScript( "demo", robotName );

	maxHp = cfg->findChild("hp")->getValueF(0);
	maxAp = cfg->findChild("ap")->getValueF(0);

	hp = cfg->findChild("hp")->getValueF(0);
	ap = cfg->findChild("ap")->getValueF(0);
	
	maxAttack = cfg->findChild("attack")->getValueF(0);
	maxDefense = cfg->findChild("defense")->getValueF(0);
	maxVelocity = cfg->findChild("velocity")->getValueF(0);

	robotNode			= TUMBU::getInstance()->getActiveSceneNode()->createChildSceneNode( robotName );
	robotPhysicsNode	= NULL;
	activeCameraNode	= TUMBU::getInstance()->getActiveSceneNode()->createChildSceneNode( robotName + "_cameraNode" );
	specialManager		= new SpecialManager( TUMBU::getInstance()->mSceneMgr, this, physicWorld );
	soundWalk			= soundManager->createSoundNode( robotNode, "walkSoundNode", "walk", "walk.ogg", true, true, true);

	punch = new Skill( "punch", this, false );
	kick = new Skill( "kick", this, false );
	jyn = new Skill( "jyn", this, false );

	skillList.push_back( punch );
	skillList.push_back( kick );
	skillList.push_back( jyn );

	objectTag = TumbuEnums::ROBOT;
}
//-------------------------------------------------------------------------------------
CharacterEnemy::~CharacterEnemy(void){
	TUMBU::getInstance()->removeCollisionDetectionListener( this );
	delete specialManager;
	
	if( charRigidBody != NULL){
		delete charRigidBody;
	}
	if( charShape != NULL ){
		delete charShape;
	}

	SoundManager::getInstance()->printAllSounds();

	soundManager->destroySound( soundWalk );

	activeCameraNode->removeAndDestroyAllChildren();
	TUMBU::getInstance()->mSceneMgr->destroySceneNode( activeCameraNode );
}
//-------------------------------------------------------------------------------------
void CharacterEnemy::movePressed(MoveKey _MoveKey) {
	switch( _MoveKey ){
	case UP:
		mKeyDirection.z = -1;
		break;
	case LEFT:
		mKeyDirection.x = -1;
		break;
	case DOWN:
		mKeyDirection.z = 1;
		break;
	case RIGHT:
		mKeyDirection.x = 1;
		break;
	case GUARD:
		if ( !isAttacking ){
			isGuard = true;

			rightArm->animationArray[ANIM_GUARD]->setEnabled(true);
			leftArm->animationArray[ANIM_GUARD]->setEnabled(true);
			legs->animationArray[ANIM_GUARD]->setEnabled(true);

			rightArm->animationArray[ANIM_GUARD]->setTimePosition(0);
			leftArm->animationArray[ANIM_GUARD]->setTimePosition(0);
			legs->animationArray[ANIM_GUARD]->setTimePosition(0);
		}
		break;
	case KICK:
		if( !isAttacking && kick->canCast() ){
			kick->cast();
			
			isAttacking = true;

			legs->animationArray[ANIM_RIGHT_KICK]->setEnabled(true);
			legs->animationArray[ANIM_RIGHT_KICK]->setTimePosition(0);
		}
		break;
	case PUNCH:
		if( !isAttacking && punch->canCast() ){
			punch->cast();
			
			isAttacking = true;

			rightArm->animationArray[ANIM_RIGHT_PUNCH]->setEnabled(true);
			rightArm->animationArray[ANIM_RIGHT_PUNCH]->setTimePosition(0);
		}
		break;
	case JYN:
		if( !isAttacking){
			if ( jyn->canCast() ){
				jyn->cast();

				rightArm->animationArray[ANIM_PRE_SPECIAL_JYN]->setEnabled(false);
				rightArm->animationArray[ANIM_PRE_SPECIAL_JYN]->setTimePosition(0);
				isAttacking = true;
			}
		}else if( jyn->canConcentrate() ){
			jyn->concentrate();

			rightArm->animationArray[ANIM_PRE_SPECIAL_JYN]->setEnabled(true);
			rightArm->animationArray[ANIM_PRE_SPECIAL_JYN]->setTimePosition(0);
		}else if( jyn->canAttack () ) {
			jyn->attack(robotNode->_getDerivedOrientation());

			rightArm->animationArray[ANIM_PRE_SPECIAL_JYN]->setEnabled(false);
			rightArm->animationArray[ANIM_PRE_SPECIAL_JYN]->setTimePosition(0);

			rightArm->animationArray[ANIM_POS_SPECIAL_JYN]->setEnabled(true);
			rightArm->animationArray[ANIM_POS_SPECIAL_JYN]->setTimePosition(0);
		}
		break;
	}

	if ( !mKeyDirection.isZeroLength() ){
		legs->animationArray[ANIM_WALK]->setEnabled(true);
		charRigidBody->getBulletObject()->activate(true);
	}
}
//-------------------------------------------------------------------------------------
void CharacterEnemy::moveReleased(CharacterEnemy::MoveKey _MoveKey) {
	switch( _MoveKey ){
	case UP:
		mKeyDirection.z = 0;
		break;
	case LEFT:
		mKeyDirection.x = 0;
		break;
	case DOWN:
		mKeyDirection.z = 0;
		break;
	case RIGHT:
		mKeyDirection.x = 0;
		break;
	case GUARD:
		isGuard = false;

		rightArm->animationArray[ANIM_GUARD]->setEnabled(false);
		leftArm->animationArray[ANIM_GUARD]->setEnabled(false);
		legs->animationArray[ANIM_GUARD]->setEnabled(false);

		rightArm->animationArray[ANIM_GUARD]->setTimePosition(0);
		leftArm->animationArray[ANIM_GUARD]->setTimePosition(0);
		legs->animationArray[ANIM_GUARD]->setTimePosition(0);

		break;
	}

	if (mKeyDirection.isZeroLength()){
		legs->animationArray[ANIM_WALK]->setEnabled(false);

		btRigidBody* rigid = charRigidBody->getBulletRigidBody();
		rigid->setAngularVelocity( btVector3(0,0,0) );
		rigid->setLinearVelocity( btVector3(0,0,0) );
	}
}
//-------------------------------------------------------------------------------------
bool CharacterEnemy::frameRenderingQueued(const Ogre::FrameEvent &evt){
	if( TUMBU::getInstance()->isPlaying() ){
		Ogre::Real time = evt.timeSinceLastFrame;

		activeCameraNode->setPosition( robotNode->getPosition() );
		specialManager->update(time);

		if( isGuard ){
			updateDefense(time);
		}else{
			updateMovement(time);
			updateAttack(time);
		}
	
		updateRegeneration(time);
	}
	return true;
}
//--------------------------------------------------------------------------
void CharacterEnemy::collision( CollisionDetectionListener *other ){
	//cout << "CharacterEnemy::Collision " << endl;
	//specialManager->collision( rigid, rigid2 );	
}
//--------------------------------------------------------------------------
OgreBulletDynamics::RigidBody* CharacterEnemy::getOgreBulletRigidBody( const std::string& instanceName ){
	return charRigidBody;
}
//-------------------------------------------------------------------------------------
void CharacterEnemy::updateMovement( const Ogre::Real time ) {
	if (!mKeyDirection.isZeroLength()){
		// Calcula a direção do movemento a paritr da camera
		mGoalDirection += mKeyDirection.z * activeCameraNode->getOrientation().zAxis();
		mGoalDirection += mKeyDirection.x * activeCameraNode->getOrientation().xAxis();
		mGoalDirection.y = 0;
		mGoalDirection.normalise();
	
		toGoal = robotNode->getOrientation().zAxis().getRotationTo(mGoalDirection);

		// Calcula quanto o char esta virado para direção de destino
		yawToGoal = toGoal.getYaw().valueDegrees();
		// Quanto pode virar nesse frame
		yawAtSpeed = yawToGoal / Ogre::Math::Abs(yawToGoal) * time * TURN_SPEED;

		// Virando o node
		if (yawToGoal < 0){
			yawToGoal = std::min<Ogre::Real>(0, std::max<Ogre::Real>(yawToGoal, yawAtSpeed));
		}else if (yawToGoal > 0){
			yawToGoal = std::max<Ogre::Real>(0, std::min<Ogre::Real>(yawToGoal, yawAtSpeed));
		}

		robotNode->yaw(Ogre::Degree(yawToGoal));
		
		localTrans.setOrigin (OgreBulletCollisions::OgreBtConverter::to(robotNode->getPosition()));
		localTrans.setRotation (OgreBulletCollisions::OgreBtConverter::to(robotNode->getOrientation()));

		charRigidBody->getBulletObject()->setWorldTransform(localTrans);

		translation = OgreBulletCollisions::OgreBtConverter::to(robotNode->_getDerivedOrientation() * Ogre::Vector3(0, 0, RUN_SPEED));
		translation.setY(0);
		charRigidBody->getBulletRigidBody()->setLinearVelocity( translation );

		legs->animationArray[ANIM_WALK]->addTime( time / 1.15f );
	}
}
//-------------------------------------------------------------------------------------
void CharacterEnemy::updateAttack( const Ogre::Real time ) {
	if( isAttacking ){
		if( jyn->isAttacking() ){
			if( jyn->special->getSpecialStatus() == SpecialInterface::CONCENTRATING ){
				rightArm->animationArray[ANIM_PRE_SPECIAL_JYN]->addTime( time );
			}else if( rightArm->animationArray[ANIM_POS_SPECIAL_JYN]->getEnabled() ){
				rightArm->animationArray[ANIM_POS_SPECIAL_JYN]->addTime(time);

				if( rightArm->animationArray[ANIM_POS_SPECIAL_JYN]->getTimePosition() >= rightArm->animationArray[ANIM_POS_SPECIAL_JYN]->getLength() ){
					if ( jyn->special->getSpecialStatus() == SpecialInterface::FINISHED){
						rightArm->animationArray[ANIM_POS_SPECIAL_JYN]->setEnabled(false);
						
						isAttacking = false;
						jyn->toDelete();
					}
				}
			}
		}
		
		if( punch->isAttacking() ){
			rightArm->animationArray[ANIM_RIGHT_PUNCH]->addTime( time );
			
			if(punch->canAttack() && 
				rightArm->animationArray[ANIM_RIGHT_PUNCH]->getTimePosition() >= rightArm->animationArray[ANIM_RIGHT_PUNCH]->getLength() / 2 ){	
				punch->attack( robotNode->getOrientation() );
			}

			if( rightArm->animationArray[ANIM_RIGHT_PUNCH]->getTimePosition() >= rightArm->animationArray[ANIM_RIGHT_PUNCH]->getLength() ){
				rightArm->animationArray[ANIM_RIGHT_PUNCH]->setEnabled(false);
				
				isAttacking = false;
				punch->setAttackFinished(true);
			}
		}

		if( kick->isAttacking() ){
			legs->animationArray[ANIM_RIGHT_KICK]->addTime( time );
			
			if(kick->canAttack() &&
				legs->animationArray[ANIM_RIGHT_KICK]->getTimePosition() >= legs->animationArray[ANIM_RIGHT_KICK]->getLength() / 2 ){	
				kick->attack(robotNode->getOrientation());
			}

			if( legs->animationArray[ANIM_RIGHT_KICK]->getTimePosition() >= legs->animationArray[ANIM_RIGHT_KICK]->getLength() ){
				legs->animationArray[ANIM_RIGHT_KICK]->setEnabled(false);

				isAttacking = false;
				kick->setAttackFinished(true);
			}
		}
	}
}
//-------------------------------------------------------------------------------------
void CharacterEnemy::updateDefense( const Ogre::Real time ) {
	rightArm->animationArray[ANIM_GUARD]->addTime(time);
	leftArm->animationArray[ANIM_GUARD]->addTime(time);
	legs->animationArray[ANIM_GUARD]->addTime(time);
}
//-------------------------------------------------------------------------------------
void CharacterEnemy::updateRegeneration( const Ogre::Real time ) {
	statsRegenartionCount += time;

	if(statsRegenartionCount >= 1){
		statsRegenartionCount -= 1;
		if( hp < getMaxHp() ) {
			if( isGuard ){
				addHp( Ogre::Real(HP_REGENERATION_PER_SECOND) * 1.75f );
			}else{
				addHp( Ogre::Real(HP_REGENERATION_PER_SECOND) );
			}
		}
		if(ap < getMaxAp()){
			if( isGuard ){
				addAp( Ogre::Real(AP_REGENERATION_PER_SECOND) * 1.75f);
			}else{
				addAp( Ogre::Real(AP_REGENERATION_PER_SECOND) );
			}
		}
	}
}
//--------------------------------------------------------------------------
void CharacterEnemy::setNode( Ogre::SceneNode* node ) {
	robotNode = node;
}
//--------------------------------------------------------------------------
void CharacterEnemy::setActiveCameraNode( Ogre::SceneNode* cameraNode ) {
}
//--------------------------------------------------------------------------
float CharacterEnemy::criarDano(float dano){
//	printf("ENEMY DANO INICIAL  = %.1f\n", dano);
	dano = dano * ( 1 + (getMaxAttack() / 10 ) );
	dano *= 10;
	dano = floor( dano );
	dano /= 10;
//	printf("ENEMY DANO CRIADO = %.1f\n", dano);
	return dano;
}
//--------------------------------------------------------------------------
float CharacterEnemy::sofrerDano(float dano){
	dano = dano * ( 1 - (getMaxDefense() / 100 ) );
	if(isGuard){
		dano /= 1.5;
	}
	dano *= 10;
	dano = floor( dano );
	dano /= 10;
	addHp( -dano );
//	printf("ENEMY DANO  RECEBIDO= %.1f\n", dano);
	return dano;
}
//--------------------------------------------------------------------------
void CharacterEnemy::addHp( float _hp ){
	hp += _hp;
	
	if( hp > getMaxHp() ){
		hp = getMaxHp();
	}

	updateHpPercent();
}
//--------------------------------------------------------------------------
void CharacterEnemy::addAp( float _ap ){
	ap += _ap;
	
	if( ap > getMaxAp() ){
		ap = getMaxAp();
	}

	updateApPercent();
}
//--------------------------------------------------------------------------
void CharacterEnemy::addHpInPercent( float hpPercent ){
	float recover = getMaxHp() * (hpPercent / 100);

	addHp( recover );
}
//--------------------------------------------------------------------------
void CharacterEnemy::addApInPercent( float apPercent ){
	float recover = getMaxAp() * (apPercent / 100);

	addAp( recover );
}
//--------------------------------------------------------------------------
void CharacterEnemy::updateHpPercent(){
	hpPercent = hp / getMaxHp();
}
//--------------------------------------------------------------------------
void CharacterEnemy::updateApPercent(){
	apPercent = ap / getMaxAp();
}
//--------------------------------------------------------------------------
void CharacterEnemy::buildAll(){
	buildNodes();
	buildCharacter();
}
//--------------------------------------------------------------------------
void CharacterEnemy::buildNodes(){

	headNode				= robotNode->createChildSceneNode( robotName + std::string("head") );
	bodyNode				= robotNode->createChildSceneNode( robotName + std::string("body") );
	rightArmNode			= robotNode->createChildSceneNode( robotName + std::string("rightArm") );
	leftArmNode				= robotNode->createChildSceneNode( robotName + std::string("leftArm") );
	legsNode				= robotNode->createChildSceneNode( robotName + std::string("legs") );

}
//--------------------------------------------------------------------------
void CharacterEnemy::resetAllAnimations(){
	
	legs->animationArray[ANIM_RIGHT_KICK]->setTimePosition(0);
	legs->animationArray[ANIM_RIGHT_KICK]->setEnabled(false);
	
	rightArm->animationArray[ANIM_GUARD]->setTimePosition(0);
	leftArm->animationArray[ANIM_GUARD]->setTimePosition(0);
	legs->animationArray[ANIM_GUARD]->setTimePosition(0);
	
	rightArm->animationArray[ANIM_GUARD]->setEnabled(false);
	leftArm->animationArray[ANIM_GUARD]->setEnabled(false);
	legs->animationArray[ANIM_GUARD]->setEnabled(false);

	rightArm->animationArray[ANIM_RIGHT_PUNCH]->setTimePosition(0);
	rightArm->animationArray[ANIM_RIGHT_PUNCH]->setEnabled(false);

	rightArm->animationArray[ANIM_PRE_SPECIAL_JYN]->setTimePosition(0);
	rightArm->animationArray[ANIM_PRE_SPECIAL_JYN]->setEnabled(false);
	
	rightArm->animationArray[ANIM_POS_SPECIAL_JYN]->setTimePosition(0);
	rightArm->animationArray[ANIM_POS_SPECIAL_JYN]->setEnabled(false);
	
	legs->animationArray[ANIM_WALK]->setEnabled(false);
	legs->animationArray[ANIM_WALK]->setTimePosition(0);
}
//--------------------------------------------------------------------------
void CharacterEnemy::stopAllSkills(){
	Skill* _skill;

	std::list<Skill*>::iterator i = skillList.begin();
	while ( i != skillList.end() ){
		_skill = (*i);
			
		_skill->toDelete();

		i++;
	}
}
//--------------------------------------------------------------------------
void CharacterEnemy::buildParts(){
	head->build();
	body->build();
	rightArm->build();
	leftArm->build();
	legs->build();

	partsBuilded = true;

	hp = getMaxHp() * hpPercent;
	ap = getMaxAp() * apPercent;
}
//--------------------------------------------------------------------------
void CharacterEnemy::unbuildParts(){
	hpPercent = hp / getMaxHp();
	apPercent = ap / getMaxAp();

	resetAllAnimations();

	head->unbuild();
	body->unbuild();
	rightArm->unbuild();
	leftArm->unbuild();
	legs->unbuild();

	partsBuilded = false;
}
//--------------------------------------------------------------------------
void CharacterEnemy::buildCharacter(){
	Ogre::Vector3 position;
	Ogre::Quaternion rotation;

	Ogre::SceneManager* mSceneMgr = TUMBU::getInstance()->mSceneMgr;
	bool castShadows = TUMBU::getInstance()->isCastShadows();

	if( charBuilded ){
		position = robotNode->getPosition();
		rotation = robotNode->getOrientation();

		delete charRigidBody;
	}

	buildParts();

	headNode->setPosition(		Ogre::Vector3( head->position ) );
	bodyNode->setPosition(		Ogre::Vector3( body->position ) );
	rightArmNode->setPosition(	Ogre::Vector3( rightArm->position ) );
	leftArmNode->setPosition(	Ogre::Vector3( leftArm->position ) );
	legsNode->setPosition(		Ogre::Vector3( legs->position ) );

	/** PHYSICS */
	charRigidBody	= new OgreBulletDynamics::RigidBody( "enemyRigid", physicWorld );
	TUMBU::getInstance()->addCollisionDetectionListener( this );

	charShape		= new OgreBulletCollisions::CompoundCollisionShape();

	charShape->addChildShape(head->collisionShape,		Ogre::Vector3( head->position ));
	charShape->addChildShape(body->collisionShape,		Ogre::Vector3( body->position ));
	charShape->addChildShape(rightArm->collisionShape,	Ogre::Vector3( rightArm->position ));
	charShape->addChildShape(leftArm->collisionShape,	Ogre::Vector3( leftArm->position ));
	charShape->addChildShape(legs->collisionShape,		Ogre::Vector3( legs->position ));

	charRigidBody->setShape( robotNode, charShape, CHAR_RESTITUTION, CHAR_FRICTION, CHAR_MASS );

	charRigidBody->getBulletRigidBody()->setSleepingThresholds( 0, 0 );
	charRigidBody->getBulletRigidBody()->setAngularFactor( 0 );

	if( !charBuilded ){
		robotNode->setPosition(0.0f, 0.2f, -8);
	}else{
		robotNode->setPosition( position );
		robotNode->setOrientation( rotation );
	}

	btTransform localTrans;		
	localTrans.setOrigin (OgreBulletCollisions::OgreBtConverter::to(robotNode->getPosition()));
	localTrans.setRotation (OgreBulletCollisions::OgreBtConverter::to(robotNode->getOrientation()));
	charRigidBody->getBulletObject()->setWorldTransform( localTrans );
	
	charBuilded = true;

	activeCameraNode->setPosition( robotNode->getPosition() );
}
//--------------------------------------------------------------------------
void CharacterEnemy::addPart( int _part, Ogre::String meshName ){
	Ogre::SceneNode* node;

	switch( _part ){
		case HEAD:
			node = headNode;
			headList.push_back(new Part( _part, meshName, node, TUMBU::getInstance()->mSceneMgr) );
		break;
		case BODY:
			node = bodyNode;
			bodyList.push_back(new Part( _part, meshName, node, TUMBU::getInstance()->mSceneMgr) );
		break;
		case RIGHT_ARM:
			node = rightArmNode;
			rightArmList.push_back(new Part( _part, meshName, node, TUMBU::getInstance()->mSceneMgr) );
		break;
		case LEFT_ARM:
			node = leftArmNode;
			leftArmList.push_back(new Part( _part, meshName, node, TUMBU::getInstance()->mSceneMgr) );
		break;
		case LEGS:
			node = legsNode;
			legsList.push_back(new Part( _part, meshName, node, TUMBU::getInstance()->mSceneMgr) );
		break;
	}
}
//--------------------------------------------------------------------------
void CharacterEnemy::addPart( Part *_part ){
	Ogre::SceneNode* node;

	switch( _part->partType ){
		case HEAD:
			node = headNode;
			_part->setNodeAndSceneManager( node, TUMBU::getInstance()->mSceneMgr );
			headList.push_back( _part );
		break;
		case BODY:
			node = bodyNode;

			_part->setNodeAndSceneManager( node, TUMBU::getInstance()->mSceneMgr );
			bodyList.push_back( _part );
		break;
		case RIGHT_ARM:
			node = rightArmNode;
			
			_part->setNodeAndSceneManager( node, TUMBU::getInstance()->mSceneMgr );
			rightArmList.push_back( _part );
		break;
		case LEFT_ARM:
			node = leftArmNode;

			_part->setNodeAndSceneManager( node, TUMBU::getInstance()->mSceneMgr );
			leftArmList.push_back( _part );
		break;
		case LEGS:
			node = legsNode;

			_part->setNodeAndSceneManager( node, TUMBU::getInstance()->mSceneMgr );
			legsList.push_back( _part );
		break;
	}
}
//--------------------------------------------------------------------------
Part* CharacterEnemy::removePart( int _part ){
	Part 
		*part,
		*returnPart,
		*__part;

	std::vector<Part*>::iterator i;

	switch( _part ){
		case HEAD:
			part = head;

			i = headList.begin();
			while ( i != headList.end() ){
				__part = (*i);
				if( part == __part ){
					headList.erase( i );
					break;
				}
				i++;
			}
		break;
		case BODY:
			part = body;

			i = bodyList.begin();
			while ( i != bodyList.end() ){
				__part = (*i);
				if( part == __part ){
					bodyList.erase( i );
					break;
				}
				i++;
			}
		break;
		case RIGHT_ARM:
			part = rightArm;

			i = rightArmList.begin();
			while ( i != rightArmList.end() ){
				__part = (*i);
				if( part == __part ){
					rightArmList.erase( i );
					break;
				}
				i++;
			}
		break;
		case LEFT_ARM:
			part = leftArm;

			i = leftArmList.begin();
			while ( i != leftArmList.end() ){
				__part = (*i);
				if( part == __part ){
					leftArmList.erase( i );
					break;
				}
				i++;
			}
		break;
		case LEGS:
			part = legs;
			
			i = legsList.begin();
			while ( i != legsList.end() ){
				__part = (*i);
				if( part == __part ){
					legsList.erase( i );
					break;
				}
				i++;
			}
		break;
	}

	returnPart = new Part( part->partType, part->setName );
	
	delete part;

	return returnPart;
}
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
float CharacterEnemy::getMaxHp(){
	float _hp = 0;

	_hp += maxHp;
	if(partsBuilded){
		_hp += head->hp;
		_hp += body->hp;
		_hp += rightArm->hp;
		_hp += leftArm->hp;
		_hp += legs->hp;
	}

	return _hp;
}
//--------------------------------------------------------------------------
float CharacterEnemy::getMaxAp(){
	float _ap = 0;

	_ap += maxAp;
	if(partsBuilded){
		_ap += head->ap;
		_ap += body->ap;
		_ap += rightArm->ap;
		_ap += leftArm->ap;
		_ap += legs->ap;
	}

	return _ap;
}
//--------------------------------------------------------------------------
float CharacterEnemy::getMaxAttack(){
	float _attack = 0;

	_attack += maxAttack;
	if(partsBuilded){
		_attack += head->attack;
		_attack += body->attack;
		_attack += rightArm->attack;
		_attack += leftArm->attack;
		_attack += legs->attack;
	}

	return _attack;
}
//--------------------------------------------------------------------------
float CharacterEnemy::getMaxDefense(){
	float _defense = 0;

	_defense += maxDefense;
	if(partsBuilded){
		_defense += head->defense;
		_defense += body->defense;
		_defense += rightArm->defense;
		_defense += leftArm->defense;
		_defense += legs->defense;
	}

	return _defense;
}
//--------------------------------------------------------------------------
float CharacterEnemy::getMaxVelocity(){
	float _velocity = 0;

	_velocity += maxVelocity;
	if(partsBuilded){
		_velocity += head->velocity;
		_velocity += body->velocity;
		_velocity += rightArm->velocity;
		_velocity += leftArm->velocity;
		_velocity += legs->velocity;
	}

	return _velocity;
}
//--------------------------------------------------------------------------
void CharacterEnemy::emmitSound( Ogre::String nodeName, Ogre::String musicName, Ogre::String musicFile, bool loop, bool stream, Ogre::Real delay ){
	Sound* soundToEmmit = soundManager->createSoundNode( robotNode, nodeName, musicName, musicFile, loop, stream, true);
	
	soundToEmmit->setDelayToPlay( delay );
}
//--------------------------------------------------------------------------
void CharacterEnemy::addEnemy(Robot* enemy){
	enemyList.push_back( enemy );
}
//--------------------------------------------------------------------------