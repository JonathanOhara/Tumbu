#include "EffectsManager.h"
#include "Effect.h"
#include "ConfigScript.h"
#include "TUMBU.h"
#include "Demo.h"
#include <algorithm>

EffectsManager* EffectsManager::instance = NULL;

// The shader side (TumbuToon.h tumbuEnergyLights) has room for this many lights.
static const int MAX_ENERGY_LIGHTS = 4;
// While a hit-stop lasts, the game runs at this fraction of real time.
static const Ogre::Real HIT_STOP_TIME_SCALE = 0.05f;
//-------------------------------------------------------------------------------------
EffectsManager::EffectsManager( Ogre::SceneManager* _sceneMgr ){
	sceneMgr = _sceneMgr;
	flashColour = Ogre::ColourValue::White;
	flashAmount = flashDuration = flashAge = 0;
	shakeAmplitude = shakeDuration = shakeAge = shakeTime = 0;
	shakeOffset = Ogre::Vector3::ZERO;
	hitStopLeft = 0;
	timeScale = 1;
	instance = this;
}
//-------------------------------------------------------------------------------------
EffectsManager::~EffectsManager(void){
	for( std::list<Effect*>::iterator i = effects.begin(); i != effects.end(); i++ ){
		delete *i;
	}
	effects.clear();

	// Nothing lit, no flash and normal time once the match is over.
	lightRequests.clear();
	updateLights();
	flashAmount = 0;
	updateScreen( 0 );
	Ogre::ControllerManager::getSingleton().setTimeFactor( 1 );
	instance = NULL;
}
//-------------------------------------------------------------------------------------
EffectsManager* EffectsManager::getInstance(void){
	return instance;
}
//-------------------------------------------------------------------------------------
Ogre::Real EffectsManager::gameTime( Ogre::Real realTime ){
	return instance != NULL ? realTime * instance->timeScale : realTime;
}
//-------------------------------------------------------------------------------------
Ogre::Vector3 EffectsManager::getShakeOffset(void){
	return instance != NULL ? instance->shakeOffset : Ogre::Vector3::ZERO;
}
//-------------------------------------------------------------------------------------
Effect* EffectsManager::spawn( const Ogre::String &name, const Ogre::Vector3 &position, const Ogre::ColourValue &kiColour, bool held ){
	ConfigNode* definition = ConfigScriptLoader::getSingleton().getConfigScript( "effect", name );
	if( definition == NULL ){
		Ogre::LogManager::getSingleton().logError( "EffectsManager: effect '" + name + "' is not defined in effects.object" );
		return NULL;
	}
	Effect* effect = new Effect( this, definition, position, kiColour, held );
	effects.push_back( effect );
	return effect;
}
//-------------------------------------------------------------------------------------
void EffectsManager::requestLight( const Ogre::Vector3 &position, const Ogre::ColourValue &colour, Ogre::Real radius ){
	LightRequest request;
	request.position = position;
	request.colour = colour;
	request.radius = radius;
	lightRequests.push_back( request );
}
//-------------------------------------------------------------------------------------
void EffectsManager::flash( const Ogre::ColourValue &colour, Ogre::Real amount, Ogre::Real duration ){
	// A stronger flash replaces a weaker one that is still fading.
	Ogre::Real current = flashDuration > 0 ? flashAmount * std::max<Ogre::Real>( 0, 1 - flashAge / flashDuration ) : 0;
	if( amount < current ){
		return;
	}
	flashColour = colour;
	flashAmount = amount;
	flashDuration = std::max<Ogre::Real>( duration, 0.01f );
	flashAge = 0;
}
//-------------------------------------------------------------------------------------
void EffectsManager::shake( const Ogre::Vector3 &source, Ogre::Real amplitude, Ogre::Real duration, Ogre::Real range ){
	// Full strength near the camera, none beyond twice the range.
	Ogre::Camera* camera = TUMBU::getInstance()->mCamera;
	Ogre::Real distance = camera->getDerivedPosition().distance( source );
	Ogre::Real falloff = range > 0 ? Ogre::Math::saturate( 2 - distance / range ) : 1;
	amplitude *= falloff;

	Ogre::Real current = shakeDuration > 0 ? shakeAmplitude * std::max<Ogre::Real>( 0, 1 - shakeAge / shakeDuration ) : 0;
	if( amplitude <= current ){
		return;
	}
	shakeAmplitude = amplitude;
	shakeDuration = std::max<Ogre::Real>( duration, 0.01f );
	shakeAge = 0;
}
//-------------------------------------------------------------------------------------
void EffectsManager::hitStop( Ogre::Real duration ){
	hitStopLeft = std::max( hitStopLeft, duration );
}
//-------------------------------------------------------------------------------------
bool EffectsManager::frameRenderingQueued( const Ogre::FrameEvent &evt ){
	Ogre::Real realTime = evt.timeSinceLastFrame;
	bool playing = TUMBU::getInstance()->isPlaying();

	// Hit-stop: game time almost stops (the particles too, through the controller time factor).
	if( hitStopLeft > 0 && playing ){
		hitStopLeft -= realTime;
	}
	timeScale = hitStopLeft > 0 ? HIT_STOP_TIME_SCALE : 1.0f;
	Ogre::ControllerManager::getSingleton().setTimeFactor( playing ? timeScale : 1.0f );

	if( playing ){
		Ogre::Real time = gameTime( realTime );
		std::list<Effect*>::iterator i = effects.begin();
		while( i != effects.end() ){
			if( (*i)->update( time ) ){
				i++;
			}else{
				delete *i;
				i = effects.erase( i );
			}
		}
		updateLights();
		updateScreen( realTime );
	}
	// Paused or flying: the lights and the flash stay as they were.
	lightRequests.clear();
	return true;
}
//-------------------------------------------------------------------------------------
static bool brighterLight( const std::pair<Ogre::Real, size_t> &a, const std::pair<Ogre::Real, size_t> &b ){
	return a.first > b.first;
}
//-------------------------------------------------------------------------------------
void EffectsManager::updateLights(void){
	// The strongest requests of this frame go to the shaders; the rest are dropped.
	std::vector< std::pair<Ogre::Real, size_t> > order;
	for( size_t i = 0; i < lightRequests.size(); i++ ){
		const Ogre::ColourValue &c = lightRequests[i].colour;
		order.push_back( std::make_pair( std::max( c.r, std::max( c.g, c.b ) ) * lightRequests[i].radius, i ) );
	}
	std::sort( order.begin(), order.end(), brighterLight );

	Ogre::GpuSharedParametersPtr params = Ogre::GpuProgramManager::getSingleton().getSharedParameters( "TumbuLighting" );
	for( int slot = 0; slot < MAX_ENERGY_LIGHTS; slot++ ){
		Ogre::Vector4 position( 0, 0, 0, 0 ), colour( 0, 0, 0, 0 );
		if( slot < (int)order.size() ){
			const LightRequest &light = lightRequests[order[slot].second];
			position = Ogre::Vector4( light.position.x, light.position.y, light.position.z, light.radius );
			colour = Ogre::Vector4( light.colour.r, light.colour.g, light.colour.b, 0 );
		}
		Ogre::String index = Ogre::StringConverter::toString( slot );
		params->setNamedConstant( "energyLightPos" + index, position );
		params->setNamedConstant( "energyLightColour" + index, colour );
	}
}
//-------------------------------------------------------------------------------------
void EffectsManager::updateScreen( Ogre::Real time ){
	// Flash: strong at once, then a quadratic fade.
	Ogre::Real flash = 0;
	if( flashAmount > 0 && flashAge < flashDuration ){
		Ogre::Real t = 1 - flashAge / flashDuration;
		flash = flashAmount * t * t;
		flashAge += time;
	}
	Ogre::GpuSharedParametersPtr params = Ogre::GpuProgramManager::getSingleton().getSharedParameters( "TumbuLighting" );
	params->setNamedConstant( "screenFlash", Ogre::Vector4( flashColour.r, flashColour.g, flashColour.b, flash ) );

	// Shake: two out-of-step waves per axis (an irregular jolt rather than a buzz), fading out.
	shakeOffset = Ogre::Vector3::ZERO;
	if( shakeAmplitude > 0 && shakeAge < shakeDuration ){
		Ogre::Real t = 1 - shakeAge / shakeDuration;
		Ogre::Real amplitude = shakeAmplitude * t * t;
		shakeTime += time;
		shakeOffset.x = amplitude * ( 0.7f * Ogre::Math::Sin( shakeTime * 61 ) + 0.3f * Ogre::Math::Sin( shakeTime * 113 + 1.3f ) );
		shakeOffset.y = amplitude * ( 0.7f * Ogre::Math::Sin( shakeTime * 53 + 2.1f ) + 0.3f * Ogre::Math::Sin( shakeTime * 97 + 0.4f ) );
		shakeAge += time;
	}
}
//-------------------------------------------------------------------------------------
Ogre::Vector3 EffectsManager::groundBelow( const Ogre::Vector3 &position ){
	Demo* demo = TUMBU::getInstance()->getDemo();
	if( demo == NULL || demo->getPhysicWorld() == NULL ){
		return position;
	}
	// The closest surface that is not a robot or an energy ball (the floor, terrain or coliseum).
	struct GroundCallback: public btCollisionWorld::ClosestRayResultCallback{
		GroundCallback( const btVector3 &from, const btVector3 &to ): btCollisionWorld::ClosestRayResultCallback( from, to ){}
		bool needsCollision( btBroadphaseProxy* proxy ) const{
			const btCollisionObject* object = (const btCollisionObject*)proxy->m_clientObject;
			const btRigidBody* body = btRigidBody::upcast( object );
			// Static surfaces only: robots and energy balls are dynamic.
			if( body != NULL && body->getInvMass() > 0 ){
				return false;
			}
			return btCollisionWorld::ClosestRayResultCallback::needsCollision( proxy );
		}
	};
	btVector3 from = Physics::OgreBtConverter::to( position + Ogre::Vector3( 0, 0.5f, 0 ) );
	btVector3 to = Physics::OgreBtConverter::to( position - Ogre::Vector3( 0, 10, 0 ) );
	GroundCallback hit( from, to );
	demo->getPhysicWorld()->getBulletCollisionWorld()->rayTest( from, to, hit );
	if( !hit.hasHit() ){
		return position;
	}
	return Physics::BtOgreConverter::to( hit.m_hitPointWorld );
}
//-------------------------------------------------------------------------------------
