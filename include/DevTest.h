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
 *   -nofx            no special-attack effects (EffectsManager::spawn does nothing)
 *   -bloomonly       the final image shows the bloom alone (x3), to see its shape and measure its flicker
 *   -faceshot        with -quitafter: the screenshot looks at the hero's face (eyes, glow, rim light);
 *                    -faceshot=jyn also charges the Jyn special just before it (eye flare)
 *   -jynwalk         -walktest that also casts and concentrates Jyn while walking; logs the ball's offset
 *                    from the point it gathers at (it must not grow while the hero moves)
 *   -fxtest=NAME     with -quitafter: start the effect "effect NAME" (effects.object) in front of the hero and
 *                    screenshot it from the side; -fxtime=S: seconds between the start and the screenshot (0.3)
 *                    -fxtest=move:NAME: held, it circles the spot (to see trails and sparks)
 *                    -fxtest=special:jyn|punch|kick: the hero uses that attack instead (fxtime before the shot)
 *                    -fxtest=special:jynthrow: charge Jyn from 1 s, throw it when ready, shot fxtime after the throw
 *                    -fxdistance=D: camera distance (4; under 3 the shot frames only the effect)
 *   -jynhit[=D]      the enemy stands still D units (6) in front of the hero (no AI) and the hero throws a Genki
 *                    Dama at it (-fxtest=special:jynthrow): screenshot of a real Jyn hit, fxtime after the throw
 *                    -jynhit=enemy: the other way round, the enemy throws its Genki Dama at the hero
 *   -camera=x,y,z,tx,ty,tz  with -quitafter: the screenshot looks from (x,y,z) at (tx,ty,tz)
 *   -strip=N         with a fixed camera (-camera or -bench): after the screenshot, pan the camera sideways by
 *                    -stripstep=D units per frame (0.003: about a third of a pixel at 15 units) and save N
 *                    consecutive frames as <workPath>/devtest-strip-NN.png (flicker and crawl of small bright details)
 *   -hero=robotNNN   the hero wears all five parts of that set (instead of the demo.object loadout)
 *   -swaptest        charge Jyn, Esc, Inventory, re-select the head (every part and its aura shell is rebuilt),
 *                    resume, charge Jyn again; logs the aura level after each step (use with -quitafter=9)
 *   -flytest         fly camera check: F, fly, Esc (no pause menu), F again; screenshot while flying
 *   -cycles=N        memory check: play N matches (3 enemy kills each, then Quit to the menu) through the
 *                    real UI, logging memory and Ogre object counts after each one, then Exit
 *
 * Every measurement is written to ogre.log with the prefix [DEVTEST].
 */
class Effect;

class DevTest: public Ogre::FrameListener{
public:
	static void parseCommandLine( const Ogre::String &commandLine );
	static bool isEnabled(void);
	/// Start hour of the in-game clock from -hour=H (fractional: 20.5 = 20:30), or -1 when not given.
	static float getStartHour(void){ return startHour; }
	/// -mute: every sound plays at volume 0 (for runs started by tools or while working on something else).
	static bool isMuted(void){ return mute; }
	/// -nofx: special-attack effects are not started (to compare performance or memory with and without them).
	static bool isFxDisabled(void){ return noFx; }
	/// -bloomonly: the final pass shows the bloom alone, x3 (Lighting sets bloomParams.w).
	static bool isBloomOnly(void){ return bloomOnly; }
	/// -aa=0|1: anti-aliasing of this run regardless of options.cfg (-1 = not given).
	static int getAntiAliasing(void){ return antiAliasing; }
	/// -sky=0|1: sky quality of this run (0 Low: no clouds, 1 High: clouds) regardless of options.cfg (-1 = not given).
	static int getSky(void){ return sky; }
	/// -hero=robotNNN: the set every hero part comes from, or empty for the demo.object loadout.
	static const Ogre::String &getHeroSet(void){ return heroSet; }

	DevTest(void);
	virtual ~DevTest(void);

	bool frameStarted( const Ogre::FrameEvent &evt );
	/// -bench: times the phases of the frame (see runBench).
	bool frameRenderingQueued( const Ogre::FrameEvent &evt );

private:
	enum Stage { WAITING_START_SCREEN, WAITING_MATCH, PLAYING, FINISHED };

	void log( const Ogre::String &message );
	void logPositions(void);
	void faceCamera( const Ogre::FrameEvent &evt );
	void placeCamera( const Ogre::Vector3 &eye, const Ogre::Vector3 &target );
	void runFxTest(void);
	bool fxSpawned;
	int fxPresses;
	Ogre::Real throwTime;
	bool jynOverLogged;
	void holdEnemy(void);
	bool enemyHeld;
	int enemyPresses;
	Ogre::Vector3 heldEnemyAt;
	Effect* fxEffect;	// the -fxtest effect (held; the match deletes it)
	Ogre::Vector3 fxSpot;
	int faceJynPresses;
	void limitFrameRate(void);
	void runGuiTour( const Ogre::FrameEvent &evt );
	void runCycles( const Ogre::FrameEvent &evt );
	void logMemory( const Ogre::String &label );
	/// -cycles: heap growth per match and object counts, logged and appended to memory-history.csv.
	void logMemorySummary(void);
	std::vector<unsigned long> heapHistory;	// heap KB at each logMemory
	std::vector<Ogre::String> objectHistory;
	void measureWalkCycles(void);
	/// -posedump: bone positions of every robot part and animation, frame by frame, into posedump.txt.
	void dumpPoses(void);
	void screenshot( const Ogre::String &name );
	void pressKey( int key );
	void releaseKey( int key );
	void runFlyTest(void);
	/// -swaptest: swap a part in the inventory while Jyn charges (the ki aura shells are rebuilt), then charge again.
	void runSwapTest(void);
	int swapStep;
	int flyStep;
	/// -bench=S: a steady frame-rate measurement (no AI, fixed camera, S seconds after a warm-up).
	void runBench(void);
	/// -strip=N: N consecutive frames while the fixed camera pans; true while it still needs frames.
	bool runStrip(void);
	int stripDone;
	bool benchStarted, benchLogged;
	unsigned long benchFrames, windowFrames;
	Ogre::Real windowMin, windowMax;
	Ogre::Timer benchTimer, windowTimer;
	Ogre::Timer phaseTimer;
	double renderMicros, restMicros;	// frameStarted -> frameRenderingQueued, and from there to the next frame
	unsigned long phaseFrames;
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
	static float startHour;
	static int cycles;
	static bool measureAnims;
	static bool poseDump;
	static bool mute;
	static int faceShot;
	static bool jynWalk;
	static bool noFx;
	static bool bloomOnly;
	static int antiAliasing;
	static int sky;
	static Ogre::Real bench;
	static bool benchAI, benchChase;
	static Ogre::String fxTest;
	static Ogre::String heroSet;
	static Ogre::Real fxTime;
	static Ogre::Real fxDistance;
	static Ogre::Real jynHit;
	static bool enemyThrows;
	static bool fixedCamera;
	static bool flyTest;
	static bool swapTest;
	static Ogre::Vector3 cameraEye, cameraTarget;
	static int stripFrames;
	static Ogre::Real stripStep;
};

#endif // #ifndef __DevTest_h_
