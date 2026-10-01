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

#endif // #ifndef __EffectLayers_h_
