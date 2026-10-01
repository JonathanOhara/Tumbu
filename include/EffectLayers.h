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
	Ogre::SceneNode* holder;	// the ribbon's own node, at the origin
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

/**
 * "converge": motes pulled into the effect from around it (the Genki Dama gathering energy). A fixed pool of
 * particles moved here every frame: each starts somewhere around the target ("from sphere": a shell in the air,
 * "from ground": a ring on the floor), swirls and accelerates in, fades in and out, then starts again. The pool
 * follows the effect, so the gathering keeps up with a walking robot. Once the effect is released, motes finish
 * their trip and the layer ends.
 */
class EffectConvergeLayer: public EffectLayer{
public:
	EffectConvergeLayer( Effect* _effect, ConfigNode* node );
	virtual ~EffectConvergeLayer(void);
	bool update( Ogre::Real age, Ogre::Real time );
	void stop(void);
private:
	struct Mote{
		Ogre::Particle* particle;
		Ogre::Vector3 offset;	// start, relative to the target
		Ogre::Real age, life, delay;
		bool alive;
	};
	void restart( Mote &mote );
	Ogre::Vector3 target(void);

	Ogre::ParticleSystem* system;
	Ogre::SceneNode* node;
	std::vector<Mote> motes;
	Ogre::ColourValue colour;
	Ogre::Real radiusMin, radiusMax, heightMin, heightMax, travel, swirl, endRadius, width, length;
	bool fromGround, toGround, stopped;
	Ogre::Vector3 lastTarget;
};

/**
 * "lightning": jagged arcs crackling over a sphere around the effect (radius x the effect's scale), rebuilt every
 * "interval" seconds; some arcs leap outwards. Drawn as a BillboardChain with an energy trail material.
 */
class EffectLightningLayer: public EffectLayer{
public:
	EffectLightningLayer( Effect* _effect, ConfigNode* node );
	virtual ~EffectLightningLayer(void);
	bool update( Ogre::Real age, Ogre::Real time );
	void stop(void);
private:
	void rebuild(void);

	Ogre::BillboardChain* chain;
	Ogre::SceneNode* holder;	// the chain's own node, at the origin
	Ogre::ColourValue colour;
	Ogre::Real radius, width, interval, chance, leap, timer;
	int arcs, segments;
	bool stopped;
};

#endif // #ifndef __EffectLayers_h_
