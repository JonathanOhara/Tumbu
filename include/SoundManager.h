#ifndef __SoundManager_h_
#define __SoundManager_h_

#include <OgreAL.h>
#include <Ogre.h>
#include <iostream>

#include "Sound.h"
using namespace std;

class SoundManager: public Ogre::FrameListener{
public:
	SoundManager(Ogre::SceneManager* sceneManager);
	virtual ~SoundManager(void);
	static SoundManager* getInstance(void);

	bool frameRenderingQueued(const Ogre::FrameEvent &evt);

	Sound* createSound(Ogre::String musicName, Ogre::String musicFile, Ogre::Vector3 position, bool loop, bool stream);
	Sound* createSoundNode(Ogre::SceneNode* father, Ogre::String nodeName, Ogre::String musicName, Ogre::String musicFile, bool loop, bool stream, bool useIdIncrement);
	
	void destroySound( Sound *sound );

	void printAllSounds();

	OgreAL::SoundManager *soundManager;

	std::list<Sound*> soundList;

	bool print;
protected:
private:
	Ogre::SceneManager* mSceneManager;

	int count;
	static SoundManager* instance;

	/** Para os iterators */
	Sound* _sound;
	std::list<Sound*>::iterator itSoundList;
	std::list<Sound*>::iterator itSoundListEnd;
};

#endif // #ifndef __SoundManager_h_