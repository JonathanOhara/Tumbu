#include "EffectsManager.h"
#include "Effect.h"
#include "ConfigScript.h"
#include "TUMBU.h"
#include "Demo.h"
#include "DevTest.h"
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
	// and no shockwave or burnt grass in the next match
	grassWaves.clear();
	grassBurns.clear();
	updateGrass( 0 );
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
	if( DevTest::isFxDisabled() ){
		return NULL;
	}
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
void EffectsManager::grassWave( const Ogre::Vector3 &position, Ogre::Real strength, Ogre::Real speed, Ogre::Real width, Ogre::Real duration ){
	GrassWave wave;
	wave.position = position;
	wave.strength = strength;
	wave.speed = speed;
	wave.width = std::max<Ogre::Real>( width, 0.1f );
	wave.duration = std::max<Ogre::Real>( duration, 0.01f );
	wave.age = 0;
	if( (int)grassWaves.size() >= GRASS_WAVES ){
		grassWaves.erase( grassWaves.begin() );
	}
	grassWaves.push_back( wave );
}
//-------------------------------------------------------------------------------------
void EffectsManager::burnGrass( const Ogre::Vector3 &position, Ogre::Real radius, Ogre::Real height, Ogre::Real embers, Ogre::Real hold, Ogre::Real regrow ){
	// only an impact near the ground burns it (not a ball that bursts high on a wall)
	const Ogre::Vector3 ground = groundBelow( position );
	if( ground == position || position.y - ground.y > height ){
		Ogre::LogManager::getSingleton().logMessage( "[Effects] no burn: the impact at " + Ogre::StringConverter::toString( position )
			+ " is not within " + Ogre::StringConverter::toString( height ) + " of the ground" );
		return;
	}
	Ogre::LogManager::getSingleton().logMessage( "[Effects] burn at " + Ogre::StringConverter::toString( ground ) + " radius "
		+ Ogre::StringConverter::toString( radius ) );
	GrassBurn burn;
	burn.position = ground;
	burn.radius = radius;
	burn.embers = embers;
	burn.hold = hold;
	burn.regrow = std::max<Ogre::Real>( regrow, 0.01f );
	burn.age = 0;
	if( (int)grassBurns.size() >= GRASS_BURNS ){
		grassBurns.erase( grassBurns.begin() );	// already fading out (below), so it does not vanish at once
	}
	grassBurns.push_back( burn );
	// The shaders take GRASS_BURNS patches: once they are all in use, the oldest grows back over a few seconds from
	// how burnt it is now, so the next burn finds it (nearly) gone instead of making it disappear.
	if( (int)grassBurns.size() == GRASS_BURNS ){
		GrassBurn &oldest = grassBurns.front();
		const Ogre::Real fade = 3.0f;
		const Ogre::Real burnt = oldest.age < oldest.hold ? 1 : 1 - ( oldest.age - oldest.hold ) / oldest.regrow;
		if( burnt > 0.01f && oldest.hold + oldest.regrow - oldest.age > fade ){
			oldest.regrow = fade / burnt;
			oldest.hold = oldest.age - ( 1 - burnt ) * oldest.regrow;
			Ogre::LogManager::getSingleton().logMessage( "[Effects] burn slots full: the oldest, at "
				+ Ogre::StringConverter::toString( oldest.position ) + ", grows back in " + Ogre::StringConverter::toString( fade ) + " s" );
		}
	}
}
//-------------------------------------------------------------------------------------
void EffectsManager::updateGrass( Ogre::Real time ){
	Ogre::GpuSharedParametersPtr params = Ogre::GpuProgramManager::getSingleton().getSharedParameters( "TumbuLighting" );
	// shockwaves: x z of the centre, the front's distance from it, its strength now (0 = none)
	std::vector<GrassWave>::iterator w = grassWaves.begin();
	while( w != grassWaves.end() ){
		w->age += time;
		if( w->age >= w->duration ){
			w = grassWaves.erase( w );
		}else{
			w++;
		}
	}
	Ogre::Vector4 width( 1, 1, 0, 0 );
	for( int slot = 0; slot < GRASS_WAVES; slot++ ){
		Ogre::Vector4 wave( 0, 0, 0, 0 );
		if( slot < (int)grassWaves.size() ){
			const GrassWave &g = grassWaves[slot];
			const Ogre::Real fade = 1 - g.age / g.duration;
			wave = Ogre::Vector4( g.position.x, g.position.z, g.speed * g.age, g.strength * fade * fade );
			width[slot] = g.width;
		}
		params->setNamedConstant( "grassWave" + Ogre::StringConverter::toString( slot ), wave );
	}

	// burnt patches: x z, radius, how burnt (0..1) plus the embers (0..1) above 1; their count goes in grassWaveWidth.z
	float data[GRASS_BURNS * 4] = { 0 };
	float heights[GRASS_BURNS * 4] = { 0 };
	std::vector<GrassBurn>::iterator b = grassBurns.begin();
	while( b != grassBurns.end() ){
		b->age += time;
		if( b->age >= b->hold + b->regrow ){
			b = grassBurns.erase( b );
		}else{
			b++;
		}
	}
	for( size_t i = 0; i < grassBurns.size() && (int)i < GRASS_BURNS; i++ ){
		const GrassBurn &g = grassBurns[i];
		const Ogre::Real burnt = g.age < g.hold ? 1 : 1 - ( g.age - g.hold ) / g.regrow;
		const Ogre::Real embers = g.embers > 0 ? std::max<Ogre::Real>( 0, 1 - g.age / g.embers ) : 0;
		data[i * 4 + 0] = g.position.x;
		data[i * 4 + 1] = g.position.z;
		data[i * 4 + 2] = g.radius;
		data[i * 4 + 3] = burnt + embers;
		heights[i * 4] = g.position.y;
	}
	width.z = (Ogre::Real)std::min<size_t>( grassBurns.size(), GRASS_BURNS );
	params->setNamedConstant( "grassWaveWidth", width );
	params->setNamedConstant( "grassBurn", data, GRASS_BURNS * 4 );
	params->setNamedConstant( "grassBurnY", heights, GRASS_BURNS * 4 );
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
		updateGrass( time );
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
