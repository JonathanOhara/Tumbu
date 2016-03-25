#include "RobotDefensiveAI.h"
#include "TUMBU.h"
#define maxX 4.5f
#define maxZ 4.5f
//-------------------------------------------------------------------------------------
RobotDefensiveAI::RobotDefensiveAI( Character* target, CharacterEnemy* _enemy ){
	enemy = _enemy;
	
	defense = false;
	movement = false;

	hNode = target->robotNode;
	eNode = enemy->robotNode;

	randomMovement();
}
//-------------------------------------------------------------------------------------
RobotDefensiveAI::~RobotDefensiveAI(void){
}
//-------------------------------------------------------------------------------------
void RobotDefensiveAI::update(const Ogre::Real time){
	timeSinceLastEvent += time;

	if( enemy->robotNode->getPosition().x == moveTarget.x 
		&& enemy->robotNode->getPosition().z == moveTarget.z){
		randomMovement();

		timeSinceLastEvent -= 0.25f;
		movement = false;
	}
}
//-------------------------------------------------------------------------------------
void RobotDefensiveAI::simulateKeyPressed(){
	if(timeSinceLastEvent >= 0.25f){

		if( movement ){
			makeMovement();
			timeSinceLastEvent -= 0.25f;
		}else{
			makeAction();
			timeSinceLastEvent -= 0.35f;
		}

		movement = !movement;
	}

}
//-------------------------------------------------------------------------------------
void RobotDefensiveAI::simulateKeyReleased(){
}
//-------------------------------------------------------------------------------------
void RobotDefensiveAI::makeAction(){
	bool attack = false;

	Ogre::Vector3 
		dirCurrent,
		dirFacing;

	Ogre::Radian angle;
	Ogre::Real angleValue;
	
	dirCurrent = eNode->_getDerivedOrientation() * Ogre::Vector3::NEGATIVE_UNIT_Z;
	dirFacing = hNode->getPosition() - eNode->getPosition();
	dirFacing.normalise();
	angle = dirCurrent .angleBetween( dirFacing );
	angleValue = angle.valueRadians();

//	cout << "angle " << angleValue << endl;	
	if( angleValue >= 2.8f && angleValue <= 3.2f ){
		if( enemy->ap >= enemy->kick->getAp() ){
			Ogre::Real kickRandom = Ogre::Math::RangeRandom(0, 100);

			if( kickRandom >= 71 ){
				enemy->movePressed( CharacterEnemy::KICK );
				attack = true;
			}
		}
	
		if( !attack && enemy->ap >= enemy->punch->getAp() ){
			Ogre::Real punchRandom = Ogre::Math::RangeRandom(0, 100);

			if( punchRandom >= 76 ){
				enemy->movePressed( CharacterEnemy::PUNCH );
				attack = true;
			}
		}

		if( attack ){
			enemy->moveReleased( CharacterEnemy::GUARD );
		}
	}
}
//--------------------------------------------------------------------------------------
void RobotDefensiveAI::makeMovement(){
	Ogre::Real defenseRandom = Ogre::Math::RangeRandom(0, 100);
	if( defenseRandom >= 66 ){
		enemy->movePressed( CharacterEnemy::GUARD );
		enemy->activeCameraNode->lookAt( Ogre::Vector3( hNode->getPosition().x, enemy->robotNode->getPosition().y, hNode->getPosition().z ), Ogre::Node::TS_WORLD );

		enemy->mKeyDirection.z = -1;
		enemy->updateMovement(0.15f);
		enemy->mKeyDirection.z = 0;
	}
	
	Ogre::Real changeTargetRandom = Ogre::Math::RangeRandom(0, 100);
	if( changeTargetRandom >= 71 ){
		enemy->moveReleased( CharacterEnemy::GUARD );
		enemy->movePressed( CharacterEnemy::UP );
		randomMovement();
	}

	Ogre::Real enemyTargetRandom = Ogre::Math::RangeRandom(0, 100);
	if( enemyTargetRandom >= 86 ){
		enemy->moveReleased( CharacterEnemy::GUARD );
		enemy->movePressed( CharacterEnemy::UP );

		moveTarget = Ogre::Vector3 ( hNode->getPosition() );
		enemy->activeCameraNode->lookAt( Ogre::Vector3( moveTarget.x, enemy->robotNode->getPosition().y, moveTarget.z ), Ogre::Node::TS_WORLD );
	}
}
//-------------------------------------------------------------------------------------
void RobotDefensiveAI::randomMovement(){
	moveTarget = Ogre::Vector3 ( Ogre::Math::RangeRandom( -maxX, maxX ), 0, Ogre::Math::RangeRandom( -maxZ, maxZ ) );
	enemy->activeCameraNode->lookAt( Ogre::Vector3( moveTarget.x, enemy->robotNode->getPosition().y, moveTarget.z ), Ogre::Node::TS_WORLD );

}
//-------------------------------------------------------------------------------------