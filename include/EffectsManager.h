#ifndef __EffectsManager_h_
#define __EffectsManager_h_

#include <Ogre.h>
#include <list>
#include <vector>

class ConfigNode;
class Effect;

/**
 * Visual effects of a match (special attacks): effects built from the "effect <name>" blocks of
 * media/configuration/effects.object, the coloured energy lights they cast on the robots and the arena (shared
 * shader parameters energyLightPos0..3 / energyLightColour0..3, see TumbuToon.h), the screen flash
 * (screenFlash, final post-processing pass), the camera shake and the hit-stop (a short slow-down of the game).
 *
 * Owned by Demo; getInstance() is NULL outside a match. Effects live here, not in the special that started them,
 * so an impact keeps playing after its projectile is gone.
 */
class EffectsManager: public Ogre::FrameListener{
public:
	EffectsManager( Ogre::SceneManager* _sceneMgr );
	virtual ~EffectsManager(void);
	/// The manager of the running match, or NULL.
	static EffectsManager* getInstance(void);

	/// Game time for this frame: the real frame time, slowed down during a hit-stop.
	static Ogre::Real gameTime( Ogre::Real realTime );
	/// Camera shake offset for this frame (camera space, world units); zero outside a match.
	static Ogre::Vector3 getShakeOffset(void);

	/**
	 * Starts the effect "effect <name>" at a position. kiColour replaces "colour ki" in the effect's layers.
	 * With held = true the caller keeps the returned pointer (to move the effect) and must call Effect::release()
	 * when done; otherwise the effect deletes itself when it has finished and the pointer must not be kept.
	 * Returns NULL when the effect is not defined.
	 */
	Effect* spawn( const Ogre::String &name, const Ogre::Vector3 &position, const Ogre::ColourValue &kiColour, bool held = false );

	/// A coloured light for this frame (colour already multiplied by its intensity). The strongest four are used.
	void requestLight( const Ogre::Vector3 &position, const Ogre::ColourValue &colour, Ogre::Real radius );
	/// Blends the screen towards a colour (amount 0..1), fading out over the given time.
	void flash( const Ogre::ColourValue &colour, Ogre::Real amount, Ogre::Real duration );
	/// Shakes the camera: amplitude in world units, full within range of the camera, none beyond twice the range.
	void shake( const Ogre::Vector3 &source, Ogre::Real amplitude, Ogre::Real duration, Ogre::Real range );
	/// Slows the whole game down (physics, robots, AI, particles) for a moment: the impact "hangs" (hit-stop).
	void hitStop( Ogre::Real duration );

	bool frameRenderingQueued( const Ogre::FrameEvent &evt );

	Ogre::SceneManager* getSceneManager(void){ return sceneMgr; }

private:
	struct LightRequest{
		Ogre::Vector3 position;
		Ogre::ColourValue colour;
		Ogre::Real radius;
	};

	void updateLights(void);
	void updateScreen( Ogre::Real time );

	Ogre::SceneManager* sceneMgr;
	std::list<Effect*> effects;
	std::vector<LightRequest> lightRequests;

	Ogre::ColourValue flashColour;
	Ogre::Real flashAmount, flashDuration, flashAge;
	Ogre::Real shakeAmplitude, shakeDuration, shakeAge, shakeTime;
	Ogre::Vector3 shakeOffset;
	Ogre::Real hitStopLeft;
	Ogre::Real timeScale;

	static EffectsManager* instance;
};

#endif // #ifndef __EffectsManager_h_
