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
	}else if( type == "trail" ){
		return new EffectTrailLayer( effect, node );
	}else if( type == "particles" ){
		return new EffectParticlesLayer( effect, node );
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
//-------------------------------------------------------------------------------------
//------------------------------------- TRAIL -----------------------------------------
//-------------------------------------------------------------------------------------
EffectTrailLayer::EffectTrailLayer( Effect* _effect, ConfigNode* node ): EffectLayer( _effect ){
	Ogre::SceneManager* sceneMgr = effect->getManager()->getSceneManager();
	Ogre::ColourValue colour = effect->readColour( node, "colour" );
	Ogre::Real width = Effect::readReal( node, "width", 0.3f );
	Ogre::Real length = Effect::readReal( node, "length", 2 );
	fadeTime = std::max<Ogre::Real>( Effect::readReal( node, "fadeTime", 0.3f ), 0.01f );
	int segments = (int)Effect::readReal( node, "segments", 20 );
	ConfigNode* material = node->findChild( "material" );

	// The tracked node starts where the effect is, so the ribbon does not stretch from the origin.
	this->node = sceneMgr->getRootSceneNode()->createChildSceneNode( effect->getPosition() );
	trail = sceneMgr->createRibbonTrail();
	trail->setMaterialName( material != NULL ? material->getValue() : "Tumbu/EnergyTrail" );
	trail->setTrailLength( length );
	trail->setMaxChainElements( segments );
	trail->setNumberOfChains( 1 );
	trail->setInitialColour( 0, colour );
	// Alpha (the shader's fade) and width go to zero over fadeTime; the colour itself stays.
	trail->setColourChange( 0, 0, 0, 0, 1.0f / fadeTime );
	trail->setInitialWidth( 0, width );
	trail->setWidthChange( 0, width / fadeTime );
	trail->setCastShadows( false );
	sceneMgr->getRootSceneNode()->attachObject( trail );
	trail->addNode( this->node );

	stopped = false;
	stoppedFor = 0;
}
//-------------------------------------------------------------------------------------
EffectTrailLayer::~EffectTrailLayer(void){
	// The trail first: it listens to the node.
	Ogre::SceneManager* sceneMgr = effect->getManager()->getSceneManager();
	sceneMgr->destroyRibbonTrail( trail );
	sceneMgr->destroySceneNode( node );
}
//-------------------------------------------------------------------------------------
bool EffectTrailLayer::update( Ogre::Real age, Ogre::Real time ){
	if( !stopped ){
		node->setPosition( effect->getPosition() );
		return true;
	}
	// Released: the node stays put and the ribbon fades out behind it.
	stoppedFor += time;
	return stoppedFor < fadeTime;
}
//-------------------------------------------------------------------------------------
void EffectTrailLayer::stop(void){
	stopped = true;
}
//-------------------------------------------------------------------------------------
//------------------------------------ PARTICLES --------------------------------------
//-------------------------------------------------------------------------------------
EffectParticlesLayer::EffectParticlesLayer( Effect* _effect, ConfigNode* node ): EffectLayer( _effect ){
	static unsigned int counter = 0;
	Ogre::SceneManager* sceneMgr = effect->getManager()->getSceneManager();
	ConfigNode* templateNode = node->findChild( "template" );
	Ogre::String templateName = templateNode != NULL ? templateNode->getValue() : "Tumbu/Fx/Sparks";
	duration = Effect::readReal( node, "time", 0 );
	// "follow 0": the system stays where the effect started (an explosion); otherwise it follows the effect.
	follow = Effect::readReal( node, "follow", 1 ) != 0;

	this->node = sceneMgr->getRootSceneNode()->createChildSceneNode( effect->getPosition() );
	system = sceneMgr->createParticleSystem( "TumbuFx" + Ogre::StringConverter::toString( counter++ ), templateName );
	system->setCastShadows( false );
	// The emitters take the effect's colour (the shader brightens it; particle colours stop at 1).
	if( node->findChild( "colour" ) != NULL ){
		Ogre::ColourValue colour = effect->readColour( node, "colour" );
		colour.r = std::min<Ogre::Real>( colour.r, 1 );
		colour.g = std::min<Ogre::Real>( colour.g, 1 );
		colour.b = std::min<Ogre::Real>( colour.b, 1 );
		for( unsigned short i = 0; i < system->getNumEmitters(); i++ ){
			system->getEmitter( i )->setColour( colour );
		}
	}
	this->node->attachObject( system );
	emitting = true;
}
//-------------------------------------------------------------------------------------
EffectParticlesLayer::~EffectParticlesLayer(void){
	Ogre::SceneManager* sceneMgr = effect->getManager()->getSceneManager();
	node->detachObject( system );
	sceneMgr->destroyParticleSystem( system );
	sceneMgr->destroySceneNode( node );
}
//-------------------------------------------------------------------------------------
bool EffectParticlesLayer::update( Ogre::Real age, Ogre::Real time ){
	if( follow ){
		node->setPosition( effect->getPosition() );
	}
	if( emitting && duration > 0 && age >= duration ){
		stop();
	}
	// Done once it stopped emitting and the last particle is gone.
	return emitting || system->getNumParticles() > 0;
}
//-------------------------------------------------------------------------------------
void EffectParticlesLayer::stop(void){
	if( emitting ){
		system->setEmitting( false );
		emitting = false;
	}
}
//-------------------------------------------------------------------------------------
