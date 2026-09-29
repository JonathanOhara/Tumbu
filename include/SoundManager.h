#ifndef __SoundManager_h_
#define __SoundManager_h_

#include <Ogre.h>
#include <iostream>
#include <list>
#include <map>

#include "Sound.h"

using namespace std;

/**
 * Audio through miniaudio (replaces OgreAL). Sound files (.ogg/.wav) are loaded through Ogre's resource
 * groups, cached in memory, and each Sound gets its own decoder. The listener follows the active camera.
 */
class SoundManager: public Ogre::FrameListener{
public:
	SoundManager( Ogre::SceneManager* sceneManager );
	virtual ~SoundManager(void);
	static SoundManager* getInstance(void);

	bool frameRenderingQueued( const Ogre::FrameEvent &evt );

	/// Sound at a fixed position; `spatial = false` for music (same volume everywhere).
	/// `stream` is kept for API compatibility (every file is decoded from memory).
	Sound* createSound( Ogre::String musicName, Ogre::String musicFile, Ogre::Vector3 position, bool loop, bool stream, bool spatial = true );
	/// 3D sound that follows a new child node of `father`.
	Sound* createSoundNode( Ogre::SceneNode* father, Ogre::String nodeName, Ogre::String musicName, Ogre::String musicFile,
		bool loop, bool stream, bool useIdIncrement );
	void destroySound( Sound *sound );
	void printAllSounds();

	std::list<Sound*> soundList;
	bool print;

private:
	Ogre::MemoryDataStreamPtr loadFile( const Ogre::String &musicFile );
	void updateListener(void);

	Ogre::SceneManager* mSceneManager;
	ma_engine *engine;
	bool engineReady;
	std::map<Ogre::String, Ogre::MemoryDataStreamPtr> fileCache;
	int count;
	static SoundManager* instance;
};

#endif // #ifndef __SoundManager_h_
