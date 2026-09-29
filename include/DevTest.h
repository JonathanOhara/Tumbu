#ifndef __DevTest_h_
#define __DevTest_h_

#include <Ogre.h>

/**
 * Developer test harness driven by command-line switches. It lets a build be checked without playing by hand:
 *
 *   -autoplay        skip the start menu, start a match and dismiss the intro dialogs
 *   -walktest        once playing: idle 1s, hold "forward" for 3s, release; log hero/enemy positions
 *   -fpscap=N        limit the frame rate to N (to compare movement at different FPS)
 *   -quitafter=S     after S seconds of play, save a screenshot to <workPath>/devtest.png and quit
 *
 * Every measurement is written to ogre.log with the prefix [DEVTEST].
 */
class DevTest: public Ogre::FrameListener{
public:
	static void parseCommandLine( const Ogre::String &commandLine );
	static bool isEnabled(void);

	DevTest(void);
	virtual ~DevTest(void);

	bool frameStarted( const Ogre::FrameEvent &evt );

private:
	enum Stage { WAITING_START_SCREEN, WAITING_MATCH, PLAYING, FINISHED };

	void log( const Ogre::String &message );
	void logPositions(void);
	void limitFrameRate(void);

	Stage stage;
	unsigned long frames;
	Ogre::Real playTime;
	Ogre::Real nextLogTime;
	bool walking;
	bool walkDone;
	Ogre::Timer frameTimer;

	static bool autoplay;
	static bool walkTest;
	static int fpsCap;
	static Ogre::Real quitAfter;
};

#endif // #ifndef __DevTest_h_
