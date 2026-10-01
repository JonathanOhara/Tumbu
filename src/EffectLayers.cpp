#include "EffectLayers.h"
#include "EffectsManager.h"
#include "ConfigScript.h"
//-------------------------------------------------------------------------------------
EffectLayer* createEffectLayer( Effect* effect, ConfigNode* node ){
	const Ogre::String &type = node->getName();
	if( type == "light" ){
		return new EffectLightLayer( effect, node );
	}else if( type == "screen" ){
		return new EffectScreenLayer( effect, node );
	}
	Ogre::LogManager::getSingleton().logWarning( "Effect: unknown layer '" + type + "' ignored" );
	return NULL;
}
//-------------------------------------------------------------------------------------
//------------------------------------- LIGHT -----------------------------------------
//-------------------------------------------------------------------------------------
EffectLightLayer::EffectLightLayer( Effect* _effect, ConfigNode* node ): EffectLayer( _effect ){
	colour		= effect->readColour( node, "colour" );
	intensity	= Effect::readReal( node, "intensity", 1 );
	radius		= Effect::readReal( node, "radius", 5 );
	rise		= Effect::readReal( node, "rise", 0.05f );
	duration	= Effect::readReal( node, "time", 0.5f );
	fade		= Effect::readReal( node, "fade", 0.25f );
	flicker		= Effect::readReal( node, "flicker", 0 );
	offset		= Effect::readVector( node, "offset", Ogre::Vector3::ZERO );
	stopped = false;
	stoppedAt = 0;
	phase = Ogre::Math::RangeRandom( 0, Ogre::Math::TWO_PI );
}
//-------------------------------------------------------------------------------------
bool EffectLightLayer::update( Ogre::Real age, Ogre::Real time ){
	Ogre::Real strength = rise > 0 ? std::min<Ogre::Real>( 1, age / rise ) : 1;
	if( duration > 0 ){
		// One-shot: fades out (quadratic) over "time" seconds after the rise.
		Ogre::Real t = ( age - rise ) / duration;
		if( t >= 1 ){
			return false;
		}
		if( t > 0 ){
			strength *= ( 1 - t ) * ( 1 - t );
		}
	}else{
		// Held: follows the effect's intensity until it is released, then fades out.
		strength *= effect->getIntensity();
		if( stopped ){
			if( stoppedAt < 0 ){
				stoppedAt = age;
			}
			Ogre::Real t = fade > 0 ? ( age - stoppedAt ) / fade : 1;
			if( t >= 1 ){
				return false;
			}
			strength *= 1 - t;
		}
	}
	if( flicker > 0 ){
		// Two out-of-step waves: an uneven crackle rather than a regular pulse.
		strength *= 1 + flicker * ( 0.6f * Ogre::Math::Sin( age * 37 + phase ) + 0.4f * Ogre::Math::Sin( age * 83 + phase * 2 ) );
	}
	if( strength > 0.001f ){
		effect->getManager()->requestLight( effect->getPosition() + offset, colour * ( intensity * strength ), radius );
	}
	return true;
}
//-------------------------------------------------------------------------------------
void EffectLightLayer::stop(void){
	stopped = true;
	stoppedAt = -1;	// set on the next update (the age is only known there)
}
//-------------------------------------------------------------------------------------
//------------------------------------- SCREEN ----------------------------------------
//-------------------------------------------------------------------------------------
EffectScreenLayer::EffectScreenLayer( Effect* _effect, ConfigNode* node ): EffectLayer( _effect ){
	flashAmount		= Effect::readReal( node, "flash", 0 );
	flashColour		= effect->readColour( node, "flashColour" );
	flashTime		= Effect::readReal( node, "flashTime", 0.12f );
	shakeAmplitude	= Effect::readReal( node, "shake", 0 );
	shakeTime		= Effect::readReal( node, "shakeTime", 0.35f );
	shakeRange		= Effect::readReal( node, "shakeRange", 12 );
	hitStop			= Effect::readReal( node, "hitStop", 0 );
}
//-------------------------------------------------------------------------------------
bool EffectScreenLayer::update( Ogre::Real age, Ogre::Real time ){
	EffectsManager* manager = effect->getManager();
	if( flashAmount > 0 ){
		manager->flash( flashColour, flashAmount, flashTime );
	}
	if( shakeAmplitude > 0 ){
		manager->shake( effect->getPosition(), shakeAmplitude, shakeTime, shakeRange );
	}
	if( hitStop > 0 ){
		manager->hitStop( hitStop );
	}
	return false;	// one-shot
}
//-------------------------------------------------------------------------------------
