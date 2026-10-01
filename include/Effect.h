#ifndef __Effect_h_
#define __Effect_h_

#include <Ogre.h>
#include <vector>

class ConfigNode;
class Effect;
class EffectsManager;

/**
 * One layer of an effect (a light, a screen flash, particles, an energy orb...), built from one block inside an
 * "effect <name>" of effects.object. Every layer reads its own values; see effects.object for the keys.
 */
class EffectLayer{
public:
	EffectLayer( Effect* _effect ): effect( _effect ){}
	virtual ~EffectLayer(void){}
	/// Advances the layer: age = seconds since the effect started, time = this frame. False once it has finished.
	virtual bool update( Ogre::Real age, Ogre::Real time ) = 0;
	/// The effect was released by its owner: looping layers wind down.
	virtual void stop(void){}
protected:
	Effect* effect;
};

/**
 * An effect started by EffectsManager::spawn: a position, a colour (the ki colour of the robot or attack) and a list
 * of layers. It finishes when every layer has finished (and, for a held effect, its owner has released it).
 */
class Effect{
public:
	Effect( EffectsManager* _manager, ConfigNode* definition, const Ogre::Vector3 &_position, const Ogre::ColourValue &_kiColour, bool _held );
	virtual ~Effect(void);

	/// False once the effect has finished and can be deleted.
	bool update( Ogre::Real time );

	/// Moves the effect (layers read the position every frame).
	void setPosition( const Ogre::Vector3 &_position ){ position = _position; }
	const Ogre::Vector3& getPosition(void) const{ return position; }
	const Ogre::ColourValue& getKiColour(void) const{ return kiColour; }
	/// Multiplies the strength of the layers that support it (lights, glow): 0..1 while an attack builds up.
	void setIntensity( Ogre::Real _intensity ){ intensity = _intensity; }
	Ogre::Real getIntensity(void) const{ return intensity; }
	EffectsManager* getManager(void){ return manager; }

	/// The owner of a held effect lets it go: looping layers stop, and the effect is deleted once they finish.
	void release(void);
	bool isReleased(void) const{ return !held; }

	/// "colour ki" / "colour ki 2" (the ki colour, times a factor) or "colour r g b"; white when missing.
	Ogre::ColourValue readColour( ConfigNode* node, const Ogre::String &key );
	/// A number, or the default when the key is missing.
	static Ogre::Real readReal( ConfigNode* node, const Ogre::String &key, Ogre::Real defaultValue );
	static Ogre::Vector3 readVector( ConfigNode* node, const Ogre::String &key, const Ogre::Vector3 &defaultValue );

private:
	EffectsManager* manager;
	std::vector<EffectLayer*> layers;
	Ogre::Vector3 position;
	Ogre::ColourValue kiColour;
	Ogre::Real age;
	Ogre::Real intensity;
	bool held;
};

#endif // #ifndef __Effect_h_
