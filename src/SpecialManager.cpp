#include "SpecialManager.h"
#include "SoundManager.h"
#include "Robot.h"
#include "TUMBU.h"
//-------------------------------------------------------------------------------------
SpecialManager::SpecialManager( Ogre::SceneManager* _sceneMgr, Robot* _robotSpeller, OgreBulletDynamics::DynamicsWorld* _world ){
	sceneMgr = _sceneMgr;
	world = _world;
	robotSpeller = _robotSpeller;
	
	specialNode = robotSpeller->robotNode->createChildSceneNode( Ogre::String(robotSpeller->robotName).append( std::string("_specialNode") ) );

	count = 0;
	//	cout << "Node name = " << specialNode->getName() << endl;
}
//-------------------------------------------------------------------------------------
SpecialManager::~SpecialManager(void){
	try{
//		cout << "node = " << specialNode->getName() << endl;
//		TUMBU::getInstance()->printSceneHierarchy();

		itSpecialList = specialList.begin();
		itSpecialListEnd = specialList.end();
		while ( itSpecialList != itSpecialListEnd ){
			_special = (*itSpecialList);

			itSpecialList = specialList.erase( itSpecialList );
			delete _special;
		}
		specialList.clear();

		specialNode->removeAndDestroyAllChildren();
		sceneMgr->destroySceneNode(specialNode);
	}catch(...){
	}
}
//-------------------------------------------------------------------------------------
void SpecialManager::update( const Ogre::Real time){
	itSpecialList = specialList.begin();
	itSpecialListEnd = specialList.end();
	while ( itSpecialList != itSpecialListEnd ){
		_special = (*itSpecialList);
		_special->update( time );

		itSpecialList++;

		if( _special->getSpecialStatus() == SpecialInterface::TO_DELETE ){
			destroySpecial( _special );
		}
	}
}
//-------------------------------------------------------------------------------------
void SpecialManager::collision(btRigidBody *rigid, btRigidBody *rigid2){
}
//-------------------------------------------------------------------------------------
Robot* SpecialManager::getTarget(){
	return robotTarget;
}
//-------------------------------------------------------------------------------------
void SpecialManager::setTarget(Robot* _robotTarget){
	robotTarget = _robotTarget;

	itSpecialList = specialList.begin();
	itSpecialListEnd = specialList.end();
	while ( itSpecialList != itSpecialListEnd ){
		_special = (*itSpecialList);
		_special->setTarget( robotTarget );
		itSpecialList++;
	}
}
//-------------------------------------------------------------------------------------
Robot* SpecialManager::getSpeller(){
	return robotSpeller;
}
//-------------------------------------------------------------------------------------
void SpecialManager::setSpeller(Robot* _robotSpeller){
	robotSpeller = _robotSpeller;

	itSpecialList = specialList.begin();
	itSpecialListEnd = specialList.end();
	while ( itSpecialList != itSpecialListEnd ){
		_special = (*itSpecialList);
		_special->setSpeller( robotSpeller );
		itSpecialList++;
	}
}
//-------------------------------------------------------------------------------------
void SpecialManager::destroySpecial( SpecialInterface* special ){
	
	itSpecialList = specialList.begin();
	itSpecialListEnd = specialList.end();
	while ( itSpecialList != itSpecialListEnd ){
		_special = (*itSpecialList);

		if( special == _special ){
			itSpecialList = specialList.erase( itSpecialList );
			delete _special;
			break;
		}
		itSpecialList++;
	}
}
//-------------------------------------------------------------------------------------
//----------------------------------- MOVES -------------------------------------------
//-------------------------------------------------------------------------------------
SpecialJyn* SpecialManager::specialJyn( float _damage ){
	count++;
	SpecialJyn* special = new SpecialJyn( sceneMgr, specialNode, robotSpeller, world, count, _damage );
	specialList.push_back(special);
	return special;
}
//-------------------------------------------------------------------------------------
SpecialPunch* SpecialManager::specialPunch( float _damage ){
	count++;
	SpecialPunch* special = new SpecialPunch( sceneMgr, specialNode, robotSpeller, world, count, _damage );
	specialList.push_back(special);
	return special;
}
//-------------------------------------------------------------------------------------
SpecialKick* SpecialManager::specialKick( float _damage ){
	count++;
	SpecialKick* special = new SpecialKick( sceneMgr, specialNode, robotSpeller, world, count, _damage );
	specialList.push_back(special);
	return special;
}
//-------------------------------------------------------------------------------------