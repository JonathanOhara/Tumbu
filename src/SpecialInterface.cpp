#include "SpecialInterface.h"
//-------------------------------------------------------------------------------------
SpecialInterface::SpecialInterface(){
	specialStatus = NONE;
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