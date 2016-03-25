#include "AIManager.h"
#include "TUMBU.h"
AIManager* AIManager::instance = NULL;
//-------------------------------------------------------------------------------------
AIManager::AIManager(){
	active = true;
}
//-------------------------------------------------------------------------------------
AIManager::~AIManager(void){
	itAiList = aiList.begin();
	itAiListEnd = aiList.end();
	while ( itAiList != itAiListEnd ){
		_robotAI = (*itAiList);

		itAiList = aiList.erase( itAiList );
		delete _robotAI;
	}
}
//-------------------------------------------------------------------------------------
AIManager* AIManager::getInstance(void){
	if( instance == NULL){
		instance = new AIManager();
	}
	return instance;
}
//-------------------------------------------------------------------------------------
void AIManager::reset(void){
	itAiList = aiList.begin();
	itAiListEnd = aiList.end();
	while ( itAiList != itAiListEnd ){
		_robotAI = (*itAiList);

		itAiList = aiList.erase( itAiList );
		delete _robotAI;
	}
}
//-------------------------------------------------------------------------------------
bool AIManager::frameRenderingQueued(const Ogre::FrameEvent& evt){
	if( active && TUMBU::getInstance()->isPlaying() ){
		itAiList = aiList.begin();
		itAiListEnd = aiList.end();
		while ( itAiList != itAiListEnd ){
			_robotAI = (*itAiList);

			_robotAI->simulateKeyPressed();
			_robotAI->update( evt.timeSinceLastFrame );
		
			if( _robotAI->getAIStatus() == RobotAI::TO_DELETE ){
				destroyRobotAI( _robotAI );
			}
			
			itAiList++;
		}
	}
	return true;
}
//-------------------------------------------------------------------------------------
void AIManager::destroyRobotAI( RobotAI* robotAI ){
	itAiList = aiList.begin();
	itAiListEnd = aiList.end();

	while ( itAiList != itAiListEnd ){
		_robotAI = (*itAiList);
		if( robotAI == _robotAI ){
			aiList.erase( itAiList );
			break;
		}
		itAiList++;
	}
}
//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------
void AIManager::createDefensiveRobotAI( Character* _target, CharacterEnemy* _enemy ){
	RobotDefensiveAI* ai = new RobotDefensiveAI( _target, _enemy );

	aiList.push_back( ai );

}
//-------------------------------------------------------------------------------------