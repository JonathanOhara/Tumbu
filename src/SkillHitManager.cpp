#include "SkillHitManager.h"
#include "GUI.h"
//-------------------------------------------------------------------------------------
SkillHitManager::SkillHitManager( GUI* _gui ){
	gui = _gui;
}
//-------------------------------------------------------------------------------------
SkillHitManager::SkillHitManager( ){
	gui = GUI::getInstance();
}
//-------------------------------------------------------------------------------------
SkillHitManager::~SkillHitManager(void){
	itSkillHitList = skillHitList.begin();
	itSkillHitListEnd = skillHitList.end();
	while ( itSkillHitList != skillHitList.end() ){
		_skillHit = (*itSkillHitList);
			
		itSkillHitList = skillHitList.erase(itSkillHitList);
		delete _skillHit;
	}

	skillHitList.clear();
}
//-------------------------------------------------------------------------------------
void SkillHitManager::reset(void){
	itSkillHitList = skillHitList.begin();
	itSkillHitListEnd = skillHitList.end();
	while ( itSkillHitList != skillHitList.end() ){
		_skillHit = (*itSkillHitList);
			
		itSkillHitList = skillHitList.erase(itSkillHitList);
		delete _skillHit;
	}

	skillHitList.clear();
}
//-------------------------------------------------------------------------------------
void SkillHitManager::update(const Ogre::Real time){
	if( skillHitList.size() > 0 ){
		itSkillHitList = skillHitList.begin();
		itSkillHitListEnd = skillHitList.end();
		while ( itSkillHitList != skillHitList.end() ){
			_skillHit = (*itSkillHitList);

			switch( _skillHit->getSkillHitStatus() ){
			
			case SkillHit::LIVE:
				_skillHit->moveUp(time);
				itSkillHitList++;
				break;
			case SkillHit::INITIALIZED:
				_skillHit->moveUp(time);
				itSkillHitList++;
				break;
			case SkillHit::CREATED:
				_skillHit->moveUp(time);
				itSkillHitList++;
				break;
			case SkillHit::TO_DELETE:
				itSkillHitList = skillHitList.erase(itSkillHitList);
				delete _skillHit;
				
				break;
			}
				
		}
	}
}
//-------------------------------------------------------------------------------------
void SkillHitManager::addSkillHit( Ogre::Vector3 position, Ogre::String message ){
	SkillHit* _skillHit = new SkillHit( position, message );
	skillHitList.push_back( _skillHit );
}
//-------------------------------------------------------------------------------------