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
 *   -measureanims    log how far the feet travel in the walk/run animations of every legs set, then quit
 *   -mute            silence all sounds (the game and every other switch work as usual)
 *   -faceshot        with -quitafter: the screenshot looks at the hero's face (eyes, glow, rim light);
 *                    -faceshot=jyn also charges the Jyn special just before it (eye flare)
 *   -jynwalk         -walktest that also casts and concentrates Jyn while walking; logs the ball's offset
 *                    from the point it gathers at (it must not grow while the hero moves)
 *   -fxtest=NAME     with -quitafter: start the effect "effect NAME" (effects.object) in front of the hero and
 *                    screenshot it from the side; -fxtime=S: seconds between the start and the screenshot (0.3)
 *                    -fxtest=special:jyn|punch|kick: the hero uses that attack instead (fxtime before the shot)
 *                    -fxdistance=D: camera distance (4; under 3 the shot frames only the effect)
 *   -camera=x,y,z,tx,ty,tz  with -quitafter: the screenshot looks from (x,y,z) at (tx,ty,tz)
 *   -flytest         fly camera check: F, fly, Esc (no pause menu), F again; screenshot while flying
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
	/// -mute: every sound plays at volume 0 (for runs started by tools or while working on something else).
	static bool isMuted(void){ return mute; }

	DevTest(void);
	virtual ~DevTest(void);

	bool frameStarted( const Ogre::FrameEvent &evt );

private:
	enum Stage { WAITING_START_SCREEN, WAITING_MATCH, PLAYING, FINISHED };

	void log( const Ogre::String &message );
	void logPositions(void);
	void faceCamera( const Ogre::FrameEvent &evt );
	void placeCamera( const Ogre::Vector3 &eye, const Ogre::Vector3 &target );
	void runFxTest(void);
	bool fxSpawned;
	int fxPresses;
	Ogre::Vector3 fxSpot;
	int faceJynPresses;
	void limitFrameRate(void);
	void runGuiTour( const Ogre::FrameEvent &evt );
	void runCycles( const Ogre::FrameEvent &evt );
	void logMemory( const Ogre::String &label );
	void measureWalkCycles(void);
	void screenshot( const Ogre::String &name );
	void pressKey( int key );
	void releaseKey( int key );
	void runFlyTest(void);
	int flyStep;
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
	static bool measureAnims;
	static bool mute;
	static int faceShot;
	static bool jynWalk;
	static Ogre::String fxTest;
	static Ogre::Real fxTime;
	static Ogre::Real fxDistance;
	static bool fixedCamera;
	static bool flyTest;
	static Ogre::Vector3 cameraEye, cameraTarget;
};

#endif // #ifndef __DevTest_h_
