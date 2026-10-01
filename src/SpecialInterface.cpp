#include "SpecialInterface.h"
#include "Robot.h"
#include "ConfigScript.h"
#include "EffectsManager.h"
#include "Effect.h"
//-------------------------------------------------------------------------------------
SpecialInterface::SpecialInterface(){
	specialStatus = NONE;
	ballEffect = NULL;
}
//-------------------------------------------------------------------------------------
SpecialInterface::~SpecialInterface(void){
	Ogre::LogManager::getSingletonPtr()->logMessage("Destroying Special");
}
//-------------------------------------------------------------------------------------
Robot* SpecialInterface::getTarget(){
	return robotTarget;
}
//-------------------------------------------------------------------------------------
void SpecialInterface::setTarget(Robot* _robotTarget){
	robotTarget = _robotTarget;
}
//-------------------------------------------------------------------------------------
Robot* SpecialInterface::getSpeller(){
	return robotSpeller;
}
//-------------------------------------------------------------------------------------
void SpecialInterface::setSpeller(Robot* _robotSpeller){
	robotSpeller = _robotSpeller;
}
//-------------------------------------------------------------------------------------
SpecialInterface::SpecialStatus SpecialInterface::getSpecialStatus(){
	return specialStatus;
}
//-------------------------------------------------------------------------------------
void SpecialInterface::setSpecialStatus(SpecialInterface::SpecialStatus _specialStatus){
	specialStatus = _specialStatus;
}
//-------------------------------------------------------------------------------------
Ogre::ColourValue SpecialInterface::getKiColour( const Ogre::String &skillName ){
	ConfigNode* cfg = ConfigScriptLoader::getSingleton().getConfigScript( "skill", skillName );
	ConfigNode* ki = cfg != NULL ? cfg->findChild( "kiColour" ) : NULL;
	if( ki != NULL && ki->getValues().size() >= 3 ){
		return Ogre::ColourValue( ki->getValueF( 0 ), ki->getValueF( 1 ), ki->getValueF( 2 ), 1 );
	}
	return robotSpeller->getKiColour();
}
//-------------------------------------------------------------------------------------
void SpecialInterface::setOrbSize( Ogre::Particle* particle, Ogre::Real diameter ){
	particle->setDimensions( diameter * ORB_SCALE, diameter * ORB_SCALE );
}
//-------------------------------------------------------------------------------------
Ogre::RGBA SpecialInterface::orbColour( const Ogre::ColourValue &ki ){
	Ogre::ColourValue colour = ki;
	colour.a = Ogre::Math::UnitRandom();
	return colour.getAsBYTE();
}
//-------------------------------------------------------------------------------------
void SpecialInterface::startBallEffect( const Ogre::String &effectName, const Ogre::Vector3 &position, const Ogre::ColourValue &ki ){
	releaseBallEffect();
	if( EffectsManager::getInstance() != NULL ){
		ballEffect = EffectsManager::getInstance()->spawn( effectName, position, ki, true );
	}
}
//-------------------------------------------------------------------------------------
void SpecialInterface::updateBallEffect( const Ogre::Vector3 &position, Ogre::Real intensity ){
	if( ballEffect != NULL ){
		ballEffect->setPosition( position );
		ballEffect->setIntensity( intensity );
	}
}
//-------------------------------------------------------------------------------------
void SpecialInterface::releaseBallEffect(void){
	if( ballEffect != NULL ){
		ballEffect->release();
		ballEffect = NULL;
	}
}
//-------------------------------------------------------------------------------------
