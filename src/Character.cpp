#include "Character.h"
#include "Util.h"
#include "TUMBU.h"
//-------------------------------------------------------------------------------------
Character::Character(OgreBulletDynamics::DynamicsWorld* _physicWorld, Ogre::String _robotName, SoundManager *_soundManager){
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

	robotNode		= TUMBU::getInstance()->getActiveSceneNode()->createChildSceneNode( robotName, Ogre::Vector3::ZERO );
	robotPhysicsNode		= TUMBU::getInstance()->getActiveSceneNode()->createChildSceneNode( robotName + "_physics", Ogre::Vector3::ZERO );
	specialManager	= new SpecialManager( TUMBU::getInstance()->mSceneMgr, this, physicWorld );
	soundWalk		= soundManager->createSoundNode( robotNode, "walkSoundNode", "walk", "walk.ogg", true, true, true);

	punch = new Skill( "punch", this, true );
	kick = new Skill( "kick", this, true );
	jyn = new Skill( "jyn", this, true );

	skillList.push_back( punch );
	skillList.push_back( kick );
	skillList.push_back( jyn );

	hpPercent = 1;
	apPercent = 1;
	aux = 0;

	objectTag = TumbuEnums::ROBOT;
}
//-------------------------------------------------------------------------------------
Character::~Character(void){
	TUMBU::getInstance()->removeCollisionDetectionListener( this );
	delete specialManager;

	if( charRigidBody != NULL){
		delete charRigidBody;
	}
	if( charShape != NULL ){
		delete charShape;
	}

	soundManager->destroySound( soundWalk );
}
//-------------------------------------------------------------------------------------
bool Character::keyPressed( const OIS::KeyEvent& input ) {
	MoveKey _moveKey = NONE;	

	if( TUMBU::getInstance()->isPlaying() ){
		switch( input.key ){
#ifdef _DEBUG
		case OIS::KC_Y:
			sofrerDano(999);
		case OIS::KC_F12:
			exit(0);
#endif
			break;
		case OIS::KC_W:
			_moveKey = UP;
			break;
		case OIS::KC_A:
			_moveKey = LEFT;
			break;
		case OIS::KC_S:
			_moveKey = DOWN;
			break;
		case OIS::KC_D:
			_moveKey = RIGHT;
			break;
		case OIS::KC_P:
			_moveKey = GUARD;
			break;
		case OIS::KC_LCONTROL:
			_moveKey = GUARD;
			break;
		case OIS::KC_U:
			_moveKey = KICK;
			break;
		case OIS::KC_1:
			_moveKey = KICK;
			break;
		case OIS::KC_O:
			_moveKey = PUNCH;
			break;
		case OIS::KC_2:
			_moveKey = PUNCH;
			break;
		case OIS::KC_I:
			_moveKey = JYN;
			break;
		case OIS::KC_3:
			_moveKey = JYN;
			break;
		case OIS::KC_LSHIFT:
			_moveKey = RUNNING;
			break;
		}
	}

	if( _moveKey != NONE ){
		movePressed( _moveKey );
	}
	return true;
}
//-------------------------------------------------------------------------------------
void Character::movePressed( MoveKey _moveKey ) {
	if( TUMBU::getInstance()->isPlaying() ){
		switch( _moveKey ){
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
		case UP_RIGHT:
			mKeyDirection.z = -1;
			mKeyDirection.x = 1;
			break;
		case UP_LEFT:
			mKeyDirection.z = -1;
			mKeyDirection.x = -1;
			break;
		case DOWN_RIGHT:
			mKeyDirection.z = 1;
			mKeyDirection.x = 1;
			break;
		case DOWN_LEFT:
			mKeyDirection.z = 1;
			mKeyDirection.x = -1;
			break;
		case RUNNING:
			if( !isAttacking ){
				isRunning = true;

				legs->animationArray[ANIM_WALK]->setEnabled( false );
				legs->animationArray[ANIM_WALK]->setTimePosition(0);
			}
			break;
		case GUARD:
			if ( !isAttacking && !isRunning ){
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
			if( !isAttacking && !isRunning ){
				if( kick->canCast() ){
					kick->cast();
			
					isAttacking = true;

					legs->animationArray[ANIM_RIGHT_KICK]->setEnabled(true);
					legs->animationArray[ANIM_RIGHT_KICK]->setTimePosition(0);
				}else{
					GUI::getInstance()->addLog( Log::FAILED, "Not Enight Ap to cast Kick!", 2 );
				}
			}
			break;
		case PUNCH:
			if( !isAttacking && !isRunning ){
				if( punch->canCast() ){
					punch->cast();
			
					isAttacking = true;

					rightArm->animationArray[ANIM_RIGHT_PUNCH]->setEnabled(true);
					rightArm->animationArray[ANIM_RIGHT_PUNCH]->setTimePosition(0);
				}else{
					GUI::getInstance()->addLog( Log::FAILED, "Not Enight Ap to cast Punch!", 2 );
				}
			}
			break;
		case JYN:
			if( !isAttacking && !isRunning ){
				if ( jyn->canCast() ){
					jyn->cast();

					rightArm->animationArray[ANIM_PRE_SPECIAL_JYN]->setEnabled(false);
					rightArm->animationArray[ANIM_PRE_SPECIAL_JYN]->setTimePosition(0);
					isAttacking = true;
				}else{
					GUI::getInstance()->addLog( Log::FAILED, "Not Enight Ap to cast Jyn!", 2 );
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
		case NONE:
			break;
		}

		if (!isGuard && !mKeyDirection.isZeroLength()){
			soundWalk->setDelayToPlay( 0 );
			charRigidBody->getBulletObject()->activate(true);

			if( isRunning ){
				legs->animationArray[ANIM_RUN]->setEnabled(true);
				leftArm->animationArray[ANIM_RUN]->setEnabled(true);
				rightArm->animationArray[ANIM_RUN]->setEnabled(true);
			}else{
				legs->animationArray[ANIM_WALK]->setEnabled(true);
			}

			charRigidBody->getBulletRigidBody()->setLinearVelocity( btVector3(0,0,0) );
		}
	}
}
//-------------------------------------------------------------------------------------
bool Character::keyReleased( const OIS::KeyEvent& input ) {
	MoveKey _moveKey = NONE;

	switch( input.key ){
	case OIS::KC_W:
		_moveKey = UP;
		break;
	case OIS::KC_A:
		_moveKey = LEFT;
		break;
	case OIS::KC_S:
		_moveKey = DOWN;
		break;
	case OIS::KC_D:
		_moveKey = RIGHT;
		break;
	case OIS::KC_P:
		_moveKey = GUARD;
		break;
	case OIS::KC_LCONTROL:
		_moveKey = GUARD;
		break;
	case OIS::KC_LSHIFT:
		_moveKey = RUNNING;
		break;
	}

	if( _moveKey != NONE ){
		moveReleased( _moveKey );
	}
	return true;
}
//-------------------------------------------------------------------------------------
void Character::moveReleased( MoveKey _moveKey ) {
	switch( _moveKey ){
	case UP:
		if( mKeyDirection.z == -1 ){
			mKeyDirection.z = 0;
		}
		break;
	case LEFT:
		if( mKeyDirection.x == -1 ){
			mKeyDirection.x = 0;
		}
		break;
	case DOWN:
		if( mKeyDirection.z == 1 ){
			mKeyDirection.z = 0;
		}
		break;
	case RIGHT:
		if( mKeyDirection.x == 1 ){
			mKeyDirection.x = 0;
		}
		break;
	case RUNNING:
		isRunning = false;

		if( !mKeyDirection.isZeroLength() ){
			legs->animationArray[ANIM_WALK]->setEnabled(true);
			legs->animationArray[ANIM_WALK]->setTimePosition(0);
		}
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
		soundWalk->stop();

		legs->animationArray[ANIM_WALK]->setEnabled(false);
		
		legs->animationArray[ANIM_RUN]->setEnabled(false);
		leftArm->animationArray[ANIM_RUN]->setEnabled(false);
		rightArm->animationArray[ANIM_RUN]->setEnabled(false);

		legs->animationArray[ANIM_WALK]->setTimePosition(0);

		legs->animationArray[ANIM_RUN]->setTimePosition(0);
		leftArm->animationArray[ANIM_RUN]->setTimePosition(0);
		rightArm->animationArray[ANIM_RUN]->setTimePosition(0);

		charRigidBody->getBulletRigidBody()->setAngularVelocity( btVector3(0,0,0) );
		charRigidBody->getBulletRigidBody()->setLinearVelocity( btVector3(0,0,0) );
	}else{
		if(!isRunning){
			legs->animationArray[ANIM_RUN]->setEnabled(false);
			leftArm->animationArray[ANIM_RUN]->setEnabled(false);
			rightArm->animationArray[ANIM_RUN]->setEnabled(false);
		}
	}
}
//-------------------------------------------------------------------------------------
void Character::releaseAllKeys(){
	isGuard = false;
	isRunning = false;

	mKeyDirection.x = 0;
	mKeyDirection.z = 0;
}
//-------------------------------------------------------------------------------------
bool Character::povMoved( const OIS::JoyStickEvent &e, int pov ) {
	int direction = e.state.mPOV[pov].direction;
	MoveKey _moveKey = UNDEFINED;

	if( TUMBU::getInstance()->isPlaying() ){
		switch( direction ){
		//North UP
		case 1:
			moveReleased( RIGHT );
			moveReleased( LEFT );
			_moveKey = UP;
			break;
		//EAST Right
		case 256:
			moveReleased( UP );
			moveReleased( DOWN );
			_moveKey = RIGHT;
			break;
		//SOUTH
		case 16:
			moveReleased( RIGHT );
			moveReleased( LEFT );
			_moveKey = DOWN;
			break;
		//WEST Left
		case 4096:
			moveReleased( UP );
			moveReleased( DOWN );
			_moveKey = LEFT;
			break;
		//NORTHEAST
		case 257:
			_moveKey = UP_RIGHT;
			break;
		//NORTHWEST
		case 4097:
			_moveKey = UP_LEFT;
			break;
		//SOUTHEAST
		case 272:
			_moveKey = DOWN_RIGHT;
			break;
		//SOUTHWEST
		case 4112:
			_moveKey = DOWN_LEFT;
			break;
		//CENTER
		case 0:
			moveReleased( UP );
			moveReleased( DOWN );
			moveReleased( RIGHT );
			moveReleased( LEFT );
			break;
		}

		if( _moveKey != UNDEFINED) {
			movePressed( _moveKey );
		}
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool Character::axisMoved( const OIS::JoyStickEvent &e, int axis ) {
	int abs = e.state.mAxes[axis].abs;

	MoveKey _moveKey = UNDEFINED;

	if( TUMBU::getInstance()->isPlaying() ){
		switch( axis ){
		case 2:
			if( abs > 5000 ){
				_moveKey = DOWN;
			}else if ( abs < -5000 ){
				_moveKey = UP;
			}else if ( abs == 0 ){
				_moveKey = NONE;
				
				moveReleased( UP );
				moveReleased( DOWN );
			}
			break;
		case 3:
			if( abs > 5000 ){
				_moveKey = RIGHT;
			}else if ( abs < -5000 ){
				_moveKey = LEFT;
			}else if ( abs == 0 ){
				_moveKey = NONE;
				moveReleased( LEFT );
				moveReleased( RIGHT );
			}
			break;
		}
		if( _moveKey != UNDEFINED) {
			movePressed( _moveKey );
		}
	}	

	return true;
}
//-------------------------------------------------------------------------------------
bool Character::sliderMoved( const OIS::JoyStickEvent &e, int sliderID ) {
	return true;
}
//------------------------------------------------------------------------------------- 
bool Character::buttonPressed( const OIS::JoyStickEvent &e, int button ) {
	MoveKey _moveKey = UNDEFINED;
	if( TUMBU::getInstance()->isPlaying() ){
		switch( button ){
		case 0:
			_moveKey = JYN;
			break;
		case 1:
			_moveKey = PUNCH;
			break;
		case 2:
			_moveKey = KICK;
			break;
		case 3:
			break;
		case 4:
			_moveKey = RUNNING;
			break;
		case 5:
			_moveKey = GUARD;
			break;

		}
		if( _moveKey != UNDEFINED) {
			movePressed( _moveKey );
		}
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool Character::buttonReleased( const OIS::JoyStickEvent &e, int button ) {
	MoveKey _moveKey = UNDEFINED;
	if( TUMBU::getInstance()->isPlaying() ){
		switch( button ){
		case 0:
			_moveKey = JYN;
			break;
		case 1:
			_moveKey = PUNCH;
			break;
		case 2:
			_moveKey = KICK;
			break;
		case 3:
			break;
		case 4:
			_moveKey = RUNNING;
			break;
		case 5:
			_moveKey = GUARD;
			break;

		}
		if( _moveKey != UNDEFINED) {
			moveReleased( _moveKey );
		}
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool Character::mouseMoved( const OIS::MouseEvent &arg ){
	return true;
}
//-------------------------------------------------------------------------------------
bool Character::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id ){
	return true;
}
//-------------------------------------------------------------------------------------
bool Character::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id ){
	return true;
}
//-------------------------------------------------------------------------------------
bool Character::frameRenderingQueued( const Ogre::FrameEvent &evt ){
	if( TUMBU::getInstance()->isPlaying() ){
		Ogre::Real time = evt.timeSinceLastFrame;
		specialManager->update(time);
	 
		if( isGuard ){
			updateDefense(time);
		}else{
			updateMovement(time);
			updateAttack(time);
		}

		Ogre::Real direction = RUN_SPEED * time;

		Ogre::Vector3 playerPos = robotNode->getPosition();

		btVector3 pos = OgreBulletCollisions::OgreBtConverter::to( robotPhysicsNode->getPosition() );

		Ogre::Vector3 position(pos.x(), pos.y(), pos.z());

		if (position != playerPos)	{
			robotNode->translate((position - playerPos) * direction);
		}

		updateRegeneration(time);
	}

	return true;
}
//--------------------------------------------------------------------------
void Character::collision( CollisionDetectionListener *other ){
	//cout << "Character::Collision " << endl;
	//specialManager->collision( rigid, rigid2 );	
}
//--------------------------------------------------------------------------
OgreBulletDynamics::RigidBody* Character::getOgreBulletRigidBody( const std::string& instanceName ){
	return charRigidBody;
}
//-------------------------------------------------------------------------------------
void Character::updateMovement( const Ogre::Real time ) {
	Ogre::Real direction = RUN_SPEED * time;

	if (!mKeyDirection.isZeroLength()){
		/*
		cout << "Roboot Node " << endl;
		cout << "x " << robotNode->getPosition().x << endl;
		cout << "y " << robotNode->getPosition().y << endl;
		cout << "z " << robotNode->getPosition().z << endl;
		*/

		mGoalDirection = Ogre::Vector3::ZERO;

		if( isRunning ){
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

			/*
			translation = OgreBulletCollisions::OgreBtConverter::to( robotNode->_getDerivedOrientation() * Ogre::Vector3(0, 0, RUN_SPEED * 1.75f ) );
			translation.setY(0);
			charRigidBody->getBulletRigidBody()->setLinearVelocity( translation );
			*/

			charRigidBody->getBulletRigidBody()->setLinearVelocity( btVector3( mGoalDirection.x * direction, mGoalDirection.y * direction, mGoalDirection.z * direction ));	

			legs->animationArray[ANIM_RUN]->addTime( time );
			leftArm->animationArray[ANIM_RUN]->addTime( time );
			rightArm->animationArray[ANIM_RUN]->addTime( time );
		} else {
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

			/*
			translation = OgreBulletCollisions::OgreBtConverter::to( robotNode->_getDerivedOrientation() * Ogre::Vector3(0, 0, RUN_SPEED ) );
			translation.setY(0);
			charRigidBody->getBulletRigidBody()->setLinearVelocity( translation );
			*/

			charRigidBody->getBulletRigidBody()->setLinearVelocity( btVector3( mGoalDirection.x * direction, mGoalDirection.y * direction, mGoalDirection.z * direction ));	

			legs->animationArray[ANIM_WALK]->addTime( time / 1.15f );
		}
	}else{
		charRigidBody->getBulletRigidBody()->setLinearVelocity( btVector3(0,0,0) );
	}
}
//-------------------------------------------------------------------------------------
void Character::updateAttack( const Ogre::Real time ) {
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
				punch->setAttackFinished( true );
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
				kick->setAttackFinished( true );
			}
		}
	}

}
//-------------------------------------------------------------------------------------
void Character::updateDefense( const Ogre::Real time ) {
	charRigidBody->getBulletRigidBody()->setLinearVelocity( btVector3(0,0,0) );
	robotNode->setPosition( robotPhysicsNode->getPosition() );
	
	rightArm->animationArray[ANIM_GUARD]->addTime(time);
	leftArm->animationArray[ANIM_GUARD]->addTime(time);
	legs->animationArray[ANIM_GUARD]->addTime(time);
}
//-------------------------------------------------------------------------------------
void Character::updateRegeneration( const Ogre::Real time ) {
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
void Character::setNode(Ogre::SceneNode* node ) {
	robotNode = node;
}
//--------------------------------------------------------------------------
void Character::setActiveCameraNode(Ogre::SceneNode* cameraNode ) {
	activeCameraNode = cameraNode;
}
//--------------------------------------------------------------------------
float Character::criarDano(float dano){
//	printf("HERO DANO INICIAL  = %.1f\n", dano);
	dano =  dano * ( 1 + (getMaxAttack() / 10 ) );
	dano *= 10;
	dano = floor( dano );
	dano /= 10;
//	printf("HERO DANO CRIADO = %.1f\n", dano);
	return dano;
}
//--------------------------------------------------------------------------
float Character::sofrerDano(float dano){
	dano = dano * ( 1 - (getMaxDefense() / 100 ) );
	if(isGuard){
		dano /= 1.5;
	}
	dano *= 10;
	dano = floor( dano );
	dano /= 10;
	addHp( -dano );
//	printf("HERO DANO  RECEBIDO= %.1f\n", dano);
	return dano;
}
//--------------------------------------------------------------------------
void Character::addHp( float _hp ){
	hp += _hp;
	
	if( hp > getMaxHp() ){
		hp = getMaxHp();
	}

	updateHpPercent();
}
//--------------------------------------------------------------------------
void Character::addAp( float _ap ){
	ap += _ap;
	
	if( ap > getMaxAp() ){
		ap = getMaxAp();
	}

	updateApPercent();
}
//--------------------------------------------------------------------------
void Character::addHpInPercent( float percent ){
	float recover = getMaxHp() * (percent / 100);

	addHp( recover );
}
//--------------------------------------------------------------------------
void Character::addApInPercent( float percent ){
	float recover = getMaxAp() * (percent / 100);

	addAp( recover );
}
//--------------------------------------------------------------------------
void Character::updateHpPercent(){
	hpPercent = hp / getMaxHp();
}
//--------------------------------------------------------------------------
void Character::updateApPercent(){
	apPercent = ap / getMaxAp();
}
//--------------------------------------------------------------------------
void Character::buildAll(){
	buildNodes();
	buildCharacter();
}
//--------------------------------------------------------------------------
void Character::buildNodes(){
	headNode		= robotNode->createChildSceneNode( robotName + std::string("head") );
	bodyNode		= robotNode->createChildSceneNode( robotName + std::string("body") );
	rightArmNode	= robotNode->createChildSceneNode( robotName + std::string("rightArm") );
	leftArmNode		= robotNode->createChildSceneNode( robotName + std::string("leftArm") );
	legsNode		= robotNode->createChildSceneNode( robotName + std::string("legs") );
}
//--------------------------------------------------------------------------
void Character::resetAllAnimations(){
	
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
void Character::stopAllSkills(){
	Skill* _skill;

	std::list<Skill*>::iterator i = skillList.begin();
	while ( i != skillList.end() ){
		_skill = (*i);
			
		_skill->toDelete();

		i++;
	}
}
//--------------------------------------------------------------------------
void Character::buildParts(){
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
void Character::unbuildParts(){
	hpPercent = hp / getMaxHp();
	apPercent = ap / getMaxAp();

	resetAllAnimations();
	stopAllSkills();

	head->unbuild();
	body->unbuild();
	rightArm->unbuild();
	leftArm->unbuild();
	legs->unbuild();

	partsBuilded = false;
}
//--------------------------------------------------------------------------
void Character::buildCharacter(){
	Ogre::Vector3 position;
	Ogre::Quaternion rotation;

	Ogre::SceneManager* mSceneMgr = TUMBU::getInstance()->mSceneMgr;
	bool castShadows = TUMBU::getInstance()->isCastShadows();

	if( charBuilded ){
		position = robotNode->getPosition();
		rotation = robotNode->getOrientation();

		delete charRigidBody;
		delete charShape;
	}

	buildParts();

	headNode->setPosition(		Ogre::Vector3( head->position ) );
	bodyNode->setPosition(		Ogre::Vector3( body->position ) );
	rightArmNode->setPosition(	Ogre::Vector3( rightArm->position ) );
	leftArmNode->setPosition(	Ogre::Vector3( leftArm->position ) );
	legsNode->setPosition(		Ogre::Vector3( legs->position ) );

	/** PHYSICS */
	charRigidBody	= new OgreBulletDynamics::RigidBody( "mainCharRigid", physicWorld );

	TUMBU::getInstance()->addCollisionDetectionListener( this );

	charShape		= new OgreBulletCollisions::CompoundCollisionShape();

	charShape->addChildShape(head->collisionShape,		Ogre::Vector3( head->position ));
	charShape->addChildShape(body->collisionShape,		Ogre::Vector3( body->position ));
	charShape->addChildShape(rightArm->collisionShape,	Ogre::Vector3( rightArm->position ));
	charShape->addChildShape(leftArm->collisionShape,	Ogre::Vector3( leftArm->position ));
	charShape->addChildShape(legs->collisionShape,		Ogre::Vector3( legs->position ));

	charRigidBody->setShape( robotPhysicsNode, charShape, CHAR_RESTITUTION, CHAR_FRICTION, CHAR_MASS );

	charRigidBody->getBulletRigidBody()->setContactProcessingThreshold( 0 );
	charRigidBody->getBulletRigidBody()->setSleepingThresholds( 0, 0 );
	charRigidBody->getBulletRigidBody()->setAngularFactor( 0 );

	if( !charBuilded ){
		charRigidBody->setPosition(0.0f, 0.2f, 8);

		robotNode->yaw(Ogre::Degree(180));
		robotNode->setPosition(0.0f, 0.2f, 8);
	}else{
		robotNode->setPosition( position );
		robotNode->setOrientation( rotation );
	}

	btTransform localTrans;
	localTrans.setOrigin (OgreBulletCollisions::OgreBtConverter::to(robotNode->getPosition()));
	localTrans.setRotation (OgreBulletCollisions::OgreBtConverter::to(robotNode->getOrientation()));
	charRigidBody->getBulletObject()->setWorldTransform( localTrans );

	charBuilded = true;
	isAttacking = false;
}
//--------------------------------------------------------------------------
void Character::addPart( int _part, Ogre::String meshName ){
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
void Character::addPart( Part *_part ){
	Ogre::SceneNode* node;

	switch( _part->partType ){
		case HEAD:
			node = headNode;

			_part->setNodeAndSceneManager( node, TUMBU::getInstance()->mSceneMgr );
			headList.push_back( _part );

			head->loadPoisition();
		break;
		case BODY:
			node = bodyNode;

			_part->setNodeAndSceneManager( node, TUMBU::getInstance()->mSceneMgr );
			bodyList.push_back( _part );
			body->loadPoisition();
		break;
		case RIGHT_ARM:
			node = rightArmNode;

			_part->setNodeAndSceneManager( node, TUMBU::getInstance()->mSceneMgr );
			rightArmList.push_back( _part );
			rightArm->loadPoisition();
		break;
		case LEFT_ARM:
			node = leftArmNode;

			_part->setNodeAndSceneManager( node, TUMBU::getInstance()->mSceneMgr );
			leftArmList.push_back( _part );
			leftArm->loadPoisition();
		break;
		case LEGS:
			node = legsNode;

			_part->setNodeAndSceneManager( node, TUMBU::getInstance()->mSceneMgr );
			legsList.push_back( _part );
			legs->loadPoisition();
		break;
	}
}
//--------------------------------------------------------------------------
Part* Character::removePart( int _part ){
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
float Character::getMaxHp(){
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
float Character::getMaxAp(){
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
float Character::getMaxAttack(){
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
float Character::getMaxDefense(){
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
float Character::getMaxVelocity(){
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
void Character::emmitSound( Ogre::String nodeName, Ogre::String musicName, Ogre::String musicFile, bool loop, bool stream, Ogre::Real delay ){
	Sound* som = soundManager->createSoundNode( robotNode, nodeName, musicName, musicFile, loop, stream, true);
	
	som->setDelayToPlay( delay );
}
//--------------------------------------------------------------------------
void Character::addEnemy(Robot* enemy){
	enemyList.push_back( enemy );
}
//--------------------------------------------------------------------------