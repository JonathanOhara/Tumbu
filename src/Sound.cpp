#include "Sound.h"
#include "TUMBU.h"
//-------------------------------------------------------------------------------------
unsigned int Sound::instances = 0;
//-------------------------------------------------------------------------------------
Sound::Sound(OgreAL::SoundManager *_soundManager, Ogre::SceneManager *_sceneManager, Ogre::SceneNode* father, Ogre::String nodeName, Ogre::String musicName, Ogre::String musicFile, bool _loop, bool stream){
	id = Sound::instances++;
	soundManager = _soundManager;
	sceneManager = _sceneManager;
	loop = _loop;

	sound = soundManager->createSound(musicName, musicFile, loop, stream);
	soundNode = father->createChildSceneNode(nodeName);
	soundNode->attachObject(sound);

	setSoundStatus( NONE );
}
//-------------------------------------------------------------------------------------
Sound::Sound(OgreAL::SoundManager *_soundManager, Ogre::SceneManager *_sceneManager, Ogre::Vector3 position, Ogre::String musicName, Ogre::String musicFile, bool _loop, bool stream){
	id = Sound::instances++;
	soundManager = _soundManager;
	sceneManager = _sceneManager;
	loop = _loop;

	sound = soundManager->createSound(musicName, musicFile, loop, stream);
	sound->setPosition( position );
	_sceneManager->getRootSceneNode()->attachObject(sound);

	soundNode = NULL;

	setSoundStatus( NONE );
}
//-------------------------------------------------------------------------------------
Sound::~Sound(void){
	try{
		sound->stop();
		soundManager->destroySound( sound );

		if( soundNode != NULL && soundNode->getName() != "" ){
			Ogre::LogManager::getSingletonPtr()->logMessage("Deleting Sound Node " + soundNode->getName() + " MUSIC " + sound->getName());
			soundNode->removeAndDestroyAllChildren();
			sceneManager->destroySceneNode( soundNode );
		}
	}catch( char * str ) {
      cout << "Exception raised: " << str << '\n';
   }
}
//-------------------------------------------------------------------------------------
OgreAL::Sound* Sound::getSound(){
	return sound;
}
//-------------------------------------------------------------------------------------
void Sound::setSound( OgreAL::Sound* _sound){
	sound = _sound;
}
//-------------------------------------------------------------------------------------
Ogre::SceneNode* Sound::getSoundNode(){
	return soundNode;
}
//-------------------------------------------------------------------------------------
void Sound::setSoundNode( Ogre::SceneNode* _soundNode ){
	soundNode = _soundNode;
}
//-------------------------------------------------------------------------------------
Sound::SoundStatus Sound::getSoundStatus(){
	return soundStatus;
}
//-------------------------------------------------------------------------------------
void Sound::setSoundStatus( Sound::SoundStatus _soundStatus ){
	soundStatus = _soundStatus;
}
//-------------------------------------------------------------------------------------
Ogre::Real Sound::getDelayToPlay(){
	return delayToPlay;
}
//-------------------------------------------------------------------------------------
void Sound::setDelayToPlay( Ogre::Real _delayToPlay ){
	try{
//		cout << "delay " << id << " name = " << soundNode->getName() << endl;

		if( getSoundStatus() != TO_DELETE ){
			setSoundStatus( READY_TO_PLAY );

			delayToPlay = _delayToPlay;

			if( !loop && sound != NULL ){
				sound->addSoundFinishedHandler(this, &Sound::soundFinished);
			}
		}
	}catch( ... ) {
      cout << "Exception setDelayToPlay: " << endl;
   }
}
//-------------------------------------------------------------------------------------
Ogre::Real Sound::removeOfDelayToPlay( Ogre::Real time ){
	delayToPlay -= time;
	return delayToPlay;
}
//-------------------------------------------------------------------------------------
void Sound::stop(){
	if( getSoundStatus() == PLAYING ){
		setSoundStatus( STOPED );
		sound->stop();
	}
}
//-------------------------------------------------------------------------------------
void Sound::play(){
	setSoundStatus( PLAYING );

	sound->play();
}
//-------------------------------------------------------------------------------------
void Sound::soundFinished(OgreAL::Sound *sound){
	setSoundStatus( TO_DELETE );
}
//-------------------------------------------------------------------------------------