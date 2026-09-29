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
 *   -guitour         screenshot every GUI screen (start menu, dialogs, HUD, pause menu tabs) and quit
 *   -hour=H          start the in-game clock at hour H (0-23) instead of 13:00, e.g. to check the night sky
 *   -cycles=N        memory check: play N matches (3 enemy kills each, then Quit to the menu) through the
 *                    real UI, logging memory and Ogre object counts after each one, then Exit
 *
 * Every measurement is written to ogre.log with the prefix [DEVTEST].
 */
class DevTest: public Ogre::FrameListener{
public:
	static void parseCommandLine( const Ogre::String &commandLine );
	static bool isEnabled(void);
	/// Start hour of the in-game clock from -hour=H, or -1 when not given.
	static int getStartHour(void){ return startHour; }

	DevTest(void);
	virtual ~DevTest(void);

	bool frameStarted( const Ogre::FrameEvent &evt );

private:
	enum Stage { WAITING_START_SCREEN, WAITING_MATCH, PLAYING, FINISHED };

	void log( const Ogre::String &message );
	void logPositions(void);
	void limitFrameRate(void);
	void runGuiTour( const Ogre::FrameEvent &evt );
	void runCycles( const Ogre::FrameEvent &evt );
	void logMemory( const Ogre::String &label );
	void screenshot( const Ogre::String &name );
	void pressKey( int key );
	void click( const Ogre::String &buttonName );
	bool isVisible( const Ogre::String &widgetName );

	int tourStep;
	int cycle;
	int kills;
	int attacks;	// attacks thrown in the current fight (-cycles)
	Ogre::Real tourTimer;

	Stage stage;
	unsigned long frames;
	Ogre::Real playTime;
	Ogre::Real nextLogTime;
	bool walking;
	bool walkDone;
	Ogre::Timer frameTimer;

	static bool autoplay;
	static bool walkTest;
	static bool guiTour;
	static int fpsCap;
	static Ogre::Real quitAfter;
	static int startHour;
	static int cycles;
};

#endif // #ifndef __DevTest_h_
