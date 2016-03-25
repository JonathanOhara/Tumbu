#include "SoundManager.h"
#include "TUMBU.h"
SoundManager* SoundManager::instance = NULL;
//-------------------------------------------------------------------------------------
SoundManager::SoundManager(Ogre::SceneManager* sceneManager){
	soundManager = new OgreAL::SoundManager();
	mSceneManager = sceneManager;

	soundList.clear();
	count = 0;

	print = false;
}
//-------------------------------------------------------------------------------------
SoundManager::~SoundManager(void){
	instance = NULL;

	itSoundList = soundList.begin();
	itSoundListEnd = soundList.end();
	while ( itSoundList != itSoundListEnd ){
		_sound = (*itSoundList);
		itSoundList = soundList.erase( itSoundList );
		delete _sound;
	}

	soundList.clear();
	soundManager->destroyAllSounds();

	delete soundManager;
}
//-------------------------------------------------------------------------------------
SoundManager* SoundManager::getInstance(){
	if( instance == NULL){
		instance = new SoundManager(TUMBU::getInstance()->mSceneMgr );
	}
	return instance;
}
//-------------------------------------------------------------------------------------
bool SoundManager::frameRenderingQueued(const Ogre::FrameEvent &evt){
	if( TUMBU::getInstance()->isPlaying() ){
		std::list<Sound*>::iterator itSoundList = soundList.begin();
		std::list<Sound*>::iterator itSoundListEnd = soundList.end();
		while ( itSoundList != itSoundListEnd ){
			_sound = (*itSoundList);

			switch ( _sound->getSoundStatus() ){
			case Sound::READY_TO_PLAY:
				if( _sound->removeOfDelayToPlay( evt.timeSinceLastFrame ) <= 0 && _sound->getSoundStatus() == Sound::READY_TO_PLAY){
					_sound->play();
				}
				itSoundList++;
				break;
			case Sound::TO_DELETE:
				itSoundList = soundList.erase( itSoundList );
				delete _sound;
				break;
			default:
				itSoundList++;
				break;
			}	
		}
	}
	return true;
}
//-------------------------------------------------------------------------------------
Sound* SoundManager::createSound(Ogre::String musicName, Ogre::String musicFile, Ogre::Vector3 position, bool loop, bool stream){
	count++;

	musicName.append( Ogre::StringConverter::toString( count ) );
	
	Sound *newSound = new Sound( soundManager, mSceneManager, position, musicName, musicFile, loop, stream);
	soundList.push_back( newSound );
	return newSound;
}
//-------------------------------------------------------------------------------------
Sound* SoundManager::createSoundNode(Ogre::SceneNode* father, Ogre::String nodeName, Ogre::String musicName, Ogre::String musicFile, bool loop, bool stream, bool useIdIncrement){
	count++;
	
	if( useIdIncrement ){
		nodeName.append( Ogre::StringConverter::toString( count ) );
		musicName.append( Ogre::StringConverter::toString( count ) );
	}

	Sound *newSound = new Sound( soundManager, mSceneManager, father, nodeName, musicName, musicFile, loop, stream);
	soundList.push_back( newSound );
	return newSound;
}
//-------------------------------------------------------------------------------------
void SoundManager::destroySound( Sound *sound ){
	Sound *_sound;
	std::list<Sound*>::iterator itSoundList = soundList.begin();
	std::list<Sound*>::iterator itSoundListEnd = soundList.end();
	while ( itSoundList != itSoundListEnd ){
		_sound = (*itSoundList);
		if( sound == _sound ){
			itSoundList = soundList.erase( itSoundList );
			delete _sound;
			break;
		}
		itSoundList++;
	}
}
//-------------------------------------------------------------------------------------
void SoundManager::printAllSounds(){
/*
	if ( print  ){
		std::list<Sound*>::iterator itSoundList = soundList.begin();
		std::list<Sound*>::iterator itSoundListEnd = soundList.end();
		while ( itSoundList != itSoundListEnd ){
			_sound = (*itSoundList);
			cout << "percorrendo = " << _sound->id << " name = " << _sound->getSoundNode()->getName() << endl;
			itSoundList++;
		}
	}
*/
}
//-------------------------------------------------------------------------------------