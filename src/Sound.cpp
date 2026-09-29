#include "Sound.h"
#include "TUMBU.h"
#include "miniaudio.h"
//-------------------------------------------------------------------------------------
unsigned int Sound::instances = 0;
//-------------------------------------------------------------------------------------
Sound::Sound( ma_engine *engine, Ogre::SceneManager *_sceneManager, Ogre::SceneNode* father, Ogre::String nodeName,
		Ogre::String musicName, const Ogre::MemoryDataStreamPtr &_data, bool _loop ){
	id = Sound::instances++;
	sceneManager = _sceneManager;
	name = musicName;
	loop = _loop;
	soundNode = father->createChildSceneNode( nodeName );
	init( engine, _data, true );
}
//-------------------------------------------------------------------------------------
Sound::Sound( ma_engine *engine, Ogre::SceneManager *_sceneManager, Ogre::Vector3 position, Ogre::String musicName,
		const Ogre::MemoryDataStreamPtr &_data, bool _loop, bool spatial ){
	id = Sound::instances++;
	sceneManager = _sceneManager;
	name = musicName;
	loop = _loop;
	soundNode = NULL;
	init( engine, _data, spatial );
	if( ready ){
		ma_sound_set_position( sound, position.x, position.y, position.z );
	}
}
//-------------------------------------------------------------------------------------
void Sound::init( ma_engine *engine, const Ogre::MemoryDataStreamPtr &_data, bool spatial ){
	data		= _data;
	decoder		= new ma_decoder;
	sound		= new ma_sound;
	ready		= false;
	delayToPlay	= 0;
	setSoundStatus( NONE );

	if( !data || engine == NULL ){
		return;
	}

	if( ma_decoder_init_memory( data->getPtr(), data->size(), NULL, decoder ) != MA_SUCCESS ){
		Ogre::LogManager::getSingleton().logError( "Sound: cannot decode " + name );
		return;
	}
	ma_uint32 flags = spatial ? 0 : MA_SOUND_FLAG_NO_SPATIALIZATION;
	if( ma_sound_init_from_data_source( engine, decoder, flags, NULL, sound ) != MA_SUCCESS ){
		ma_decoder_uninit( decoder );
		Ogre::LogManager::getSingleton().logError( "Sound: cannot create " + name );
		return;
	}
	ma_sound_set_looping( sound, loop ? MA_TRUE : MA_FALSE );
	// Robots are about 2 units tall: full volume within 2 units, then a gentle roll-off.
	ma_sound_set_min_distance( sound, 2.0f );
	ma_sound_set_rolloff( sound, 0.5f );
	ready = true;
	update();
}
//-------------------------------------------------------------------------------------
Sound::~Sound(void){
	if( ready ){
		ma_sound_uninit( sound );
		ma_decoder_uninit( decoder );
	}
	delete sound;
	delete decoder;

	if( soundNode != NULL ){
		soundNode->removeAndDestroyAllChildren();
		sceneManager->destroySceneNode( soundNode );
	}
}
//-------------------------------------------------------------------------------------
void Sound::update(void){
	if( !ready ){
		return;
	}
	if( soundNode != NULL ){
		Ogre::Vector3 position = soundNode->_getDerivedPosition();
		ma_sound_set_position( sound, position.x, position.y, position.z );
	}
	if( getSoundStatus() == PLAYING && !loop && ma_sound_at_end( sound ) ){
		setSoundStatus( TO_DELETE );
	}
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
	if( getSoundStatus() != TO_DELETE ){
		setSoundStatus( READY_TO_PLAY );
		delayToPlay = _delayToPlay;
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
		if( ready ){
			ma_sound_stop( sound );
		}
	}
}
//-------------------------------------------------------------------------------------
void Sound::play(){
	setSoundStatus( PLAYING );
	if( ready ){
		ma_sound_seek_to_pcm_frame( sound, 0 );
		ma_sound_start( sound );
	}else if( !loop ){
		setSoundStatus( TO_DELETE );	// could not be loaded: let the manager clean it up
	}
}
//-------------------------------------------------------------------------------------
