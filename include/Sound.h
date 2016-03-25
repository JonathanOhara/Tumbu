#ifndef __Sound_h_
#define __Sound_h_

#include <OgreAL.h>
#include <Ogre.h>
#include <iostream>

using namespace std;

class Sound{
public:
	Sound(OgreAL::SoundManager *_soundManager, Ogre::SceneManager *_sceneManager, Ogre::SceneNode* father, Ogre::String nodeName, Ogre::String musicName, Ogre::String musicFile, bool _loop, bool stream);
	Sound(OgreAL::SoundManager *_soundManager, Ogre::SceneManager *_sceneManager, Ogre::Vector3 position, Ogre::String musicName, Ogre::String musicFile, bool _loop, bool stream);
	virtual ~Sound(void);

	OgreAL::Sound* getSound();
	void setSound( OgreAL::Sound* _sound );

	Ogre::SceneNode* getSoundNode();
	void setSoundNode( Ogre::SceneNode* _soundNode );

	Ogre::Real getDelayToPlay();
	void setDelayToPlay( Ogre::Real _delayToPlay );
	
	void soundFinished(OgreAL::Sound *sound);

	Ogre::Real removeOfDelayToPlay(Ogre::Real time);

	void stop();
	void play();

	enum SoundStatus { NONE, STOPED, PLAYING, READY_TO_PLAY, TO_DELETE };

	SoundStatus getSoundStatus();
	void setSoundStatus( SoundStatus _soundStatus );

	int id;
protected:

private:
	OgreAL::SoundManager *soundManager;
	Ogre::SceneManager *sceneManager;
	Ogre::SceneNode *soundNode;
	OgreAL::Sound *sound;

	bool loop;

	Ogre::Real delayToPlay;

	SoundStatus soundStatus;

	static unsigned int instances;
};

#endif // #ifndef __Sound_h_