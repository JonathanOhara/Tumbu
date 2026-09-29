#include "SoundManager.h"
#include "TUMBU.h"
#include "miniaudio.h"
//-------------------------------------------------------------------------------------
SoundManager* SoundManager::instance = NULL;
//-------------------------------------------------------------------------------------
SoundManager::SoundManager( Ogre::SceneManager* sceneManager ){
	mSceneManager	= sceneManager;
	count			= 0;
	print			= false;
	engine			= new ma_engine;
	engineReady		= ma_engine_init( NULL, engine ) == MA_SUCCESS;

	if( engineReady ){
		ma_device *device = ma_engine_get_device( engine );
		Ogre::LogManager::getSingleton().logMessage( Ogre::String( "Sound: miniaudio " MA_VERSION_STRING " on '" ) +
			device->playback.name + "', " + Ogre::StringConverter::toString( ma_engine_get_sample_rate( engine ) ) + " Hz" );
	}else{
		Ogre::LogManager::getSingleton().logError( "Sound: no audio device, the game runs silent" );
	}
}
//-------------------------------------------------------------------------------------
SoundManager::~SoundManager(void){
	instance = NULL;
	while( !soundList.empty() ){
		delete soundList.front();
		soundList.pop_front();
	}
	if( engineReady ){
		ma_engine_uninit( engine );
	}
	delete engine;
}
//-------------------------------------------------------------------------------------
SoundManager* SoundManager::getInstance(){
	if( instance == NULL ){
		instance = new SoundManager( TUMBU::getInstance()->mSceneMgr );
	}
	return instance;
}
//-------------------------------------------------------------------------------------
Ogre::MemoryDataStreamPtr SoundManager::loadFile( const Ogre::String &musicFile ){
	std::map<Ogre::String, Ogre::MemoryDataStreamPtr>::iterator it = fileCache.find( musicFile );
	if( it != fileCache.end() ){
		return it->second;
	}
	Ogre::MemoryDataStreamPtr data;
	try{
		Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource( musicFile,
			Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
		data.reset( new Ogre::MemoryDataStream( stream ) );
	}catch( Ogre::Exception &e ){
		Ogre::LogManager::getSingleton().logError( "Sound: " + e.getDescription() );
	}
	fileCache[ musicFile ] = data;
	return data;
}
//-------------------------------------------------------------------------------------
void SoundManager::updateListener(void){
	if( !engineReady ){
		return;
	}
	Ogre::Camera *camera = TUMBU::getInstance()->mCamera;
	if( camera == NULL || !camera->isAttached() ){
		return;
	}
	Ogre::Vector3 position = camera->getDerivedPosition();
	Ogre::Vector3 direction = camera->getDerivedDirection();
	Ogre::Vector3 up = camera->getDerivedUp();
	ma_engine_listener_set_position( engine, 0, position.x, position.y, position.z );
	ma_engine_listener_set_direction( engine, 0, direction.x, direction.y, direction.z );
	ma_engine_listener_set_world_up( engine, 0, up.x, up.y, up.z );
}
//-------------------------------------------------------------------------------------
bool SoundManager::frameRenderingQueued( const Ogre::FrameEvent &evt ){
	updateListener();

	std::list<Sound*>::iterator it = soundList.begin();
	while( it != soundList.end() ){
		Sound *sound = *it;
		sound->update();

		switch( sound->getSoundStatus() ){
		case Sound::READY_TO_PLAY:
			if( TUMBU::getInstance()->isPlaying() && sound->removeOfDelayToPlay( evt.timeSinceLastFrame ) <= 0 ){
				sound->play();
			}
			++it;
			break;
		case Sound::TO_DELETE:
			it = soundList.erase( it );
			delete sound;
			break;
		default:
			++it;
			break;
		}
	}
	return true;
}
//-------------------------------------------------------------------------------------
Sound* SoundManager::createSound( Ogre::String musicName, Ogre::String musicFile, Ogre::Vector3 position, bool loop, bool stream, bool spatial ){
	count++;
	musicName.append( Ogre::StringConverter::toString( count ) );
	Sound *newSound = new Sound( engineReady ? engine : NULL, mSceneManager, position, musicName, loadFile( musicFile ), loop, spatial );
	soundList.push_back( newSound );
	return newSound;
}
//-------------------------------------------------------------------------------------
Sound* SoundManager::createSoundNode( Ogre::SceneNode* father, Ogre::String nodeName, Ogre::String musicName, Ogre::String musicFile,
		bool loop, bool stream, bool useIdIncrement ){
	count++;
	if( useIdIncrement ){
		nodeName.append( Ogre::StringConverter::toString( count ) );
		musicName.append( Ogre::StringConverter::toString( count ) );
	}
	Sound *newSound = new Sound( engineReady ? engine : NULL, mSceneManager, father, nodeName, musicName, loadFile( musicFile ), loop );
	soundList.push_back( newSound );
	return newSound;
}
//-------------------------------------------------------------------------------------
void SoundManager::destroySound( Sound *sound ){
	std::list<Sound*>::iterator it = std::find( soundList.begin(), soundList.end(), sound );
	if( it != soundList.end() ){
		soundList.erase( it );
		delete sound;
	}
}
//-------------------------------------------------------------------------------------
void SoundManager::destroySoundsUnder( Ogre::SceneNode *node ){
	// Sounds attached below a node that is about to be destroyed (a robot's punch/kick sounds that are still
	// waiting or playing): without this they would keep a dangling node and never be deleted.
	std::list<Sound*>::iterator it = soundList.begin();
	while( it != soundList.end() ){
		bool under = false;
		for( Ogre::Node *n = (*it)->getSoundNode(); n != NULL; n = n->getParent() ){
			if( n == node ){
				under = true;
				break;
			}
		}
		if( under ){
			delete *it;
			it = soundList.erase( it );
		}else{
			++it;
		}
	}
}
//-------------------------------------------------------------------------------------
void SoundManager::printAllSounds(){
}
//-------------------------------------------------------------------------------------
