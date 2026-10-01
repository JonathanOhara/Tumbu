#ifndef __EffectLayers_h_
#define __EffectLayers_h_

#include "Effect.h"

/// Builds the layer for one block of an effect definition ("light", "screen", ...); NULL for an unknown block.
EffectLayer* createEffectLayer( Effect* effect, ConfigNode* node );

/**
 * "light": a coloured point light on the robots and the arena (EffectsManager::requestLight). It rises to full
 * strength, then fades out over "time" seconds; with "time 0" it stays (times the effect's intensity) until the
 * effect is released, then fades out over "fade" seconds.
 */
class EffectLightLayer: public EffectLayer{
public:
	EffectLightLayer( Effect* _effect, ConfigNode* node );
	bool update( Ogre::Real age, Ogre::Real time );
	void stop(void);
private:
	Ogre::ColourValue colour;
	Ogre::Vector3 offset;
	Ogre::Real intensity, radius, rise, duration, fade, flicker;
	Ogre::Real stoppedAt;
	Ogre::Real phase;
	bool stopped;
};

/// "screen": a one-shot screen flash, camera shake and hit-stop when the effect starts.
class EffectScreenLayer: public EffectLayer{
public:
	EffectScreenLayer( Effect* _effect, ConfigNode* node );
	bool update( Ogre::Real age, Ogre::Real time );
private:
	Ogre::ColourValue flashColour;
	Ogre::Real flashAmount, flashTime, shakeAmplitude, shakeTime, shakeRange, hitStop;
};

/**
 * "trail": a ribbon left behind by the moving effect (Ogre RibbonTrail), in the effect's colour. Each point fades
 * out and narrows over "fadeTime" seconds; once the effect is released the ribbon dies out and the layer ends.
 */
class EffectTrailLayer: public EffectLayer{
public:
	EffectTrailLayer( Effect* _effect, ConfigNode* node );
	virtual ~EffectTrailLayer(void);
	bool update( Ogre::Real age, Ogre::Real time );
	void stop(void);
private:
	Ogre::RibbonTrail* trail;
	Ogre::SceneNode* node;
	Ogre::Real fadeTime;
	Ogre::Real stoppedFor;
	bool stopped;
};

/**
 * "particles": an Ogre particle system following the effect; its emitters take the
 * effect's colour. With "time" > 0 it emits for that long (a burst); otherwise until the effect is released. The
 * layer ends when no particle is left. "offset x y z" moves it from the effect; "ground 1" puts it on the floor below.
 * A one-shot (not held) effect emits for 0.2 s when "time" is not given.
 */
class EffectParticlesLayer: public EffectLayer{
public:
	EffectParticlesLayer( Effect* _effect, ConfigNode* node );
	virtual ~EffectParticlesLayer(void);
	bool update( Ogre::Real age, Ogre::Real time );
	void stop(void);
private:
	Ogre::ParticleSystem* system;
	Ogre::SceneNode* node;
	Ogre::Real duration;
	bool follow;
	Ogre::Vector3 offset;
	bool onGround;
	bool emitting;
};

#endif // #ifndef __EffectLayers_h_
