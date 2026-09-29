#ifndef __Sound_h_
#define __Sound_h_

#include <Ogre.h>
#include <iostream>

using namespace std;

struct ma_engine;
struct ma_sound;
struct ma_decoder;

/**
 * One playing (or waiting) sound, backed by miniaudio. Created through SoundManager.
 * A sound either follows a scene node (3D, e.g. footsteps on a robot) or sits at a fixed position; fixed
 * sounds can be non-spatial (music). Non-looping sounds mark themselves TO_DELETE when they finish and the
 * SoundManager frees them.
 */
class Sound{
public:
	Sound( ma_engine *engine, Ogre::SceneManager *_sceneManager, Ogre::SceneNode* father, Ogre::String nodeName,
		Ogre::String musicName, const Ogre::MemoryDataStreamPtr &data, bool _loop );
	Sound( ma_engine *engine, Ogre::SceneManager *_sceneManager, Ogre::Vector3 position, Ogre::String musicName,
		const Ogre::MemoryDataStreamPtr &data, bool _loop, bool spatial );
	virtual ~Sound(void);

	Ogre::SceneNode* getSoundNode();
	void setSoundNode( Ogre::SceneNode* _soundNode );

	Ogre::Real getDelayToPlay();
	void setDelayToPlay( Ogre::Real _delayToPlay );
	Ogre::Real removeOfDelayToPlay(Ogre::Real time);

	void stop();
	void play();
	/// Moves node-attached sounds and detects the end of non-looping sounds (called every frame).
	void update(void);

	enum SoundStatus { NONE, STOPED, PLAYING, READY_TO_PLAY, TO_DELETE };
	SoundStatus getSoundStatus();
	void setSoundStatus( SoundStatus _soundStatus );

	const Ogre::String& getName(void) const { return name; }
	int id;

private:
	void init( ma_engine *engine, const Ogre::MemoryDataStreamPtr &data, bool spatial );

	Ogre::SceneManager *sceneManager;
	Ogre::SceneNode *soundNode;
	Ogre::MemoryDataStreamPtr data;	// encoded file, kept alive for the decoder
	ma_decoder *decoder;
	ma_sound *sound;
	Ogre::String name;
	bool loop;
	bool ready;
	Ogre::Real delayToPlay;
	SoundStatus soundStatus;
	static unsigned int instances;
};

#endif // #ifndef __Sound_h_
