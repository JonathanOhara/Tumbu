#include "Skill.h"
#include "ConfigScript.h"
#include "Robot.h"
#include "TUMBU.h"
#include "GUI.h"
//-------------------------------------------------------------------------------------
Skill::Skill( Ogre::String _skillID, Robot *_robot, bool _showLog ){
	skillID = _skillID;
	robot = _robot;
	showLog = _showLog;
	specialManager = robot->specialManager;

	experience = 0;
	level = 1;

	ConfigNode* cfg;
	cfg = ConfigScriptLoader::getSingleton().getConfigScript( "skill", skillID );

	damage = cfg->findChild("damage")->getValueI();
	ap = cfg->findChild("ap")->getValueI();
	energyBalls = cfg->findChild("energyBalls")->getValueI();
	experienceMultiplier = cfg->findChild("experience_multiplier")->getValueF();

	cfg = ConfigScriptLoader::getSingleton().getConfigScript( "skill", "experience" );
	experienceNextLevel = cfg->findChild( Ogre::String("level").append( Ogre::StringConverter::toString(level+1) ) )->getValueI();

	special = NULL;
	attackFinished = true;
}
//-------------------------------------------------------------------------------------
Skill::~Skill(void){
}
//-------------------------------------------------------------------------------------
void Skill::cast(){
	robot->addAp( -ap );

	if( skillID == "punch" ){
		special = specialManager->specialPunch( damage );
	}else if( skillID == "kick" ){
		special = specialManager->specialKick( damage );
	}else if ( skillID == "jyn" ){
		special = specialManager->specialJyn( damage );
	}

	attackFinished = false;
	experience += 1;

	verifyLevelUp();
}
//-------------------------------------------------------------------------------------
bool Skill::canCast(){
	bool returnValue = false;
	
	if( special == NULL ){
		returnValue = true;
	}
	
	if( !returnValue && attackFinished ){
		returnValue = true;
	}

	if( robot->ap < ap ){
		returnValue = false;
	}

	return returnValue;
}
//-------------------------------------------------------------------------------------
void Skill::concentrate(){
	special->concentrate();
}
//-------------------------------------------------------------------------------------
bool Skill::canConcentrate(){
	bool returnValue = false;
	if( !attackFinished ){
		if( special->getSpecialStatus() == SpecialInterface::NONE ){
			returnValue = true;
		}
	}

	return returnValue;
}
//-------------------------------------------------------------------------------------
void Skill::attack(Ogre::Quaternion _orientation){
	if( special->getSpecialStatus() == SpecialInterface::CONCENTRATED ){
		special->attack(_orientation);
	}
}
//-------------------------------------------------------------------------------------
bool Skill::canAttack(){
	bool returnValue = false;
	if( !attackFinished ){
		if( special->getSpecialStatus() == SpecialInterface::CONCENTRATED ){
			returnValue = true;
		}
	}
	return returnValue;
}
//-------------------------------------------------------------------------------------
void Skill::toDelete(){
	if( !attackFinished ){
		if(special != NULL){
			special->toDelete();
		}
		special = NULL;
		attackFinished = true;
	}
}
//-------------------------------------------------------------------------------------
void Skill::verifyLevelUp(){
	if(experience >= experienceNextLevel){
		ConfigNode* cfg;
		cfg = ConfigScriptLoader::getSingleton().getConfigScript( "skill", "experience" );
		
		level++;

		experienceNextLevel = cfg->findChild( Ogre::String("level").append( Ogre::StringConverter::toString(level+1) ) )->getValueI();

		cfg = ConfigScriptLoader::getSingleton().getConfigScript( "skill", skillID );

		damage += cfg->findChild("one_lv_up_damage")->getValueI();
		ap += cfg->findChild("one_lv_up_ap")->getValueI();
		energyBalls += cfg->findChild("one_lv_up_energyBalls")->getValueI();

		if( showLog ){
			GUI::getInstance()->addLog( Log::LEVEL_UP, "Skill " + skillID +" Level: " + Ogre::StringConverter::toString( level ).c_str() + "!", 5 );
		}
	}
}
//-------------------------------------------------------------------------------------
int Skill::getDamage(){
	return damage;
}
//-------------------------------------------------------------------------------------
void Skill::setDamage( int _damage ){
	damage = _damage;
}
//-------------------------------------------------------------------------------------
int Skill::getEnergyBalls(){
	return energyBalls;
}
//-------------------------------------------------------------------------------------
void Skill::setEnergyBalls( int _energyBalls ){
	energyBalls = _energyBalls;
}
//-------------------------------------------------------------------------------------
int Skill::getAp(){
	return ap;
}
//-------------------------------------------------------------------------------------
void Skill::setAp( int _ap ){
	ap = _ap;
}
//-------------------------------------------------------------------------------------
int Skill::getLevel(){
	return level;
}
//-------------------------------------------------------------------------------------
void Skill::setLevel( int _level ){
	level = _level;
}
//-------------------------------------------------------------------------------------
int Skill::getExperience(){
	return experience;
}
//-------------------------------------------------------------------------------------
void Skill::setExperience( int _experience ){
	experience = _experience;
}
//-------------------------------------------------------------------------------------
int Skill::getExperienceNextLevel(){
	return experienceNextLevel;
}
//-------------------------------------------------------------------------------------
void Skill::setExperienceNextLevel( int _experienceNextLevel ){
	experienceNextLevel = _experienceNextLevel;
}
//-------------------------------------------------------------------------------------
bool Skill::isAttacking(){
	return !attackFinished;
}
//-------------------------------------------------------------------------------------
void Skill::setAttackFinished( bool _attackFinished ){
	attackFinished = _attackFinished;
}
//-------------------------------------------------------------------------------------