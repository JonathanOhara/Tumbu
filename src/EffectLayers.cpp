#include "EffectLayers.h"
#include "EffectsManager.h"
#include "ConfigScript.h"
#include <OgreParticleSystemRenderer.h>
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
	}else if( type == "converge" ){
		return new EffectConvergeLayer( effect, node );
	}else if( type == "lightning" ){
		return new EffectLightningLayer( effect, node );
	}else if( type == "aura" ){
		return NULL;	// the robot's glow shell: read and drawn by Robot::updateAura, not a layer of the effect
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
	holder = sceneMgr->getRootSceneNode()->createChildSceneNode();
	holder->attachObject( trail );
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
	sceneMgr->destroySceneNode( holder );
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
	offset = Effect::readVector( node, "offset", Ogre::Vector3::ZERO );
	onGround = Effect::readReal( node, "ground", 0 ) != 0;
	if( duration <= 0 && effect->isReleased() ){
		duration = 0.2f;	// one-shot effect: a burst, so the layer always ends
	}
	Ogre::Vector3 start = effect->getPosition();
	if( onGround ){
		start = effect->getManager()->groundBelow( start ) + Ogre::Vector3( 0, 0.03f, 0 );
	}

	this->node = sceneMgr->getRootSceneNode()->createChildSceneNode( start + offset );
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
		node->setPosition( effect->getPosition()  + offset );
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
//-------------------------------------------------------------------------------------
//------------------------------------ CONVERGE ---------------------------------------
//-------------------------------------------------------------------------------------
static void readRange( ConfigNode* node, const Ogre::String &key, Ogre::Real &min, Ogre::Real &max ){
	ConfigNode* child = node->findChild( key );
	if( child == NULL || child->getValues().empty() ){
		return;
	}
	min = child->getValueF( 0 );
	max = child->getValues().size() > 1 ? child->getValueF( 1 ) : min;
}
//-------------------------------------------------------------------------------------
EffectConvergeLayer::EffectConvergeLayer( Effect* _effect, ConfigNode* node ): EffectLayer( _effect ){
	Ogre::SceneManager* sceneMgr = effect->getManager()->getSceneManager();
	ConfigNode* material = node->findChild( "material" );
	ConfigNode* billboard = node->findChild( "billboard" );
	ConfigNode* from = node->findChild( "from" );
	colour = effect->readColour( node, "colour" );
	colour.r = std::min<Ogre::Real>( colour.r, 1 );
	colour.g = std::min<Ogre::Real>( colour.g, 1 );
	colour.b = std::min<Ogre::Real>( colour.b, 1 );
	fromGround = from != NULL && from->getValue() == "ground";
	toGround = Effect::readReal( node, "toGround", 0 ) != 0;
	radiusMin = 2; radiusMax = 4;
	readRange( node, "radius", radiusMin, radiusMax );
	heightMin = -1; heightMax = 1;
	readRange( node, "height", heightMin, heightMax );
	width = 0.05f; length = 0.3f;
	readRange( node, "size", width, length );
	travel = std::max<Ogre::Real>( Effect::readReal( node, "time", 1 ), 0.1f );
	swirl = Effect::readReal( node, "swirl", 1 );
	endRadius = Effect::readReal( node, "endRadius", 0 );
	int count = (int)Effect::readReal( node, "count", 30 );
	stopped = false;

	// Particles in world coordinates, moved here only: no emitter, and speed 0 (so Ogre neither moves nor ages them).
	this->node = sceneMgr->getRootSceneNode()->createChildSceneNode();
	system = sceneMgr->createParticleSystem( count );
	system->setMaterialName( material != NULL ? material->getValue() : "Tumbu/EnergySpark" );
	// A renderer setting (the particle system itself ignores it).
	system->getRenderer()->setParameter( "billboard_type", billboard != NULL ? billboard->getValue() : "oriented_self" );
	system->setDefaultDimensions( width, length );
	system->setSpeedFactor( 0 );
	system->setCastShadows( false );
	system->setCullIndividually( false );
	this->node->attachObject( system );
	system->_update( 0 );

	lastTarget = target();
	for( int i = 0; i < count; i++ ){
		Mote mote;
		mote.particle = system->createParticle();
		if( mote.particle == NULL ){
			break;
		}
		mote.particle->mTimeToLive = 1e6f;
		mote.particle->mTotalTimeToLive = 1e6f;
		mote.particle->setDimensions( 0, 0 );
		restart( mote );
		// Spread the first trips out, so the motes do not all arrive together.
		mote.delay = Ogre::Math::RangeRandom( 0, travel * 1.2f );
		motes.push_back( mote );
	}
}
//-------------------------------------------------------------------------------------
EffectConvergeLayer::~EffectConvergeLayer(void){
	Ogre::SceneManager* sceneMgr = effect->getManager()->getSceneManager();
	node->detachObject( system );
	sceneMgr->destroyParticleSystem( system );
	sceneMgr->destroySceneNode( node );
}
//-------------------------------------------------------------------------------------
Ogre::Vector3 EffectConvergeLayer::target(void){
	if( toGround ){
		return effect->getManager()->groundBelow( effect->getPosition() ) + Ogre::Vector3( 0, 0.05f, 0 );
	}
	return effect->getPosition();
}
//-------------------------------------------------------------------------------------
void EffectConvergeLayer::restart( Mote &mote ){
	Ogre::Real distance = Ogre::Math::RangeRandom( radiusMin, radiusMax );
	Ogre::Radian angle( Ogre::Math::RangeRandom( 0, Ogre::Math::TWO_PI ) );
	Ogre::Vector3 offset( Ogre::Math::Cos( angle ) * distance, 0, Ogre::Math::Sin( angle ) * distance );
	if( fromGround ){
		// On the floor around the target, a little above it.
		Ogre::Vector3 goal = target();
		Ogre::Vector3 ground = effect->getManager()->groundBelow( goal + offset );
		offset = ground - goal + Ogre::Vector3( 0, Ogre::Math::RangeRandom( heightMin, heightMax ), 0 );
	}else{
		offset.y = Ogre::Math::RangeRandom( heightMin, heightMax );
	}
	mote.offset = offset;
	mote.age = 0;
	mote.delay = 0;
	mote.life = travel * Ogre::Math::RangeRandom( 0.75f, 1.25f );
	mote.alive = true;
}
//-------------------------------------------------------------------------------------
bool EffectConvergeLayer::update( Ogre::Real age, Ogre::Real time ){
	Ogre::Vector3 goal = target();
	Ogre::Real strength = Ogre::Math::saturate( effect->getIntensity() );
	Ogre::Real reach = endRadius * effect->getScale();
	bool any = false;
	for( size_t i = 0; i < motes.size(); i++ ){
		Mote &mote = motes[i];
		if( !mote.alive ){
			continue;
		}
		if( mote.delay > 0 ){
			mote.delay -= time;
			if( stopped ){
				mote.alive = false;
			}else{
				any = true;
			}
			continue;
		}
		mote.age += time;
		Ogre::Real t = mote.age / mote.life;
		if( t >= 1 ){
			if( stopped ){
				mote.alive = false;
				mote.particle->setDimensions( 0, 0 );
				continue;
			}
			restart( mote );
			t = 0;
		}
		any = true;
		// Sucked in: slow at first, fast at the end, turning around the target.
		Ogre::Real pull = 1 - t * t;
		Ogre::Quaternion turn( Ogre::Radian( swirl * t ), Ogre::Vector3::UNIT_Y );
		Ogre::Vector3 offset = turn * mote.offset;
		Ogre::Real distance = offset.length();
		Ogre::Vector3 position = goal;
		if( distance > 0.0001f ){
			position += offset * ( ( reach + ( distance - reach ) * pull ) / distance );
		}
		Ogre::Vector3 heading = position - mote.particle->mPosition;
		mote.particle->mPosition = position;
		if( heading.squaredLength() > 1e-8f ){
			mote.particle->mDirection = heading.normalisedCopy();
		}
		// Fades in and out; brighter as the attack builds up.
		Ogre::ColourValue c = colour;
		c.a = Ogre::Math::saturate( t / 0.2f ) * Ogre::Math::saturate( ( 1 - t ) / 0.15f ) * ( 0.4f + 0.6f * strength );
		mote.particle->mColour = c.getAsBYTE();
		mote.particle->setDimensions( width, length * ( 0.6f + 0.8f * t ) );
	}
	return any || !stopped;
}
//-------------------------------------------------------------------------------------
void EffectConvergeLayer::stop(void){
	stopped = true;
}
//-------------------------------------------------------------------------------------
//------------------------------------ LIGHTNING --------------------------------------
//-------------------------------------------------------------------------------------
EffectLightningLayer::EffectLightningLayer( Effect* _effect, ConfigNode* node ): EffectLayer( _effect ){
	Ogre::SceneManager* sceneMgr = effect->getManager()->getSceneManager();
	ConfigNode* material = node->findChild( "material" );
	colour = effect->readColour( node, "colour" );
	colour.r = std::min<Ogre::Real>( colour.r, 1 );
	colour.g = std::min<Ogre::Real>( colour.g, 1 );
	colour.b = std::min<Ogre::Real>( colour.b, 1 );
	radius = Effect::readReal( node, "radius", 1.1f );
	width = Effect::readReal( node, "width", 0.04f );
	interval = std::max<Ogre::Real>( Effect::readReal( node, "interval", 0.06f ), 0.016f );
	chance = Effect::readReal( node, "chance", 0.6f );
	leap = Effect::readReal( node, "leap", 0.3f );
	arcs = std::max( 1, (int)Effect::readReal( node, "arcs", 3 ) );
	segments = std::max( 2, (int)Effect::readReal( node, "segments", 8 ) );
	timer = 0;
	stopped = false;

	chain = sceneMgr->createBillboardChain();
	chain->setNumberOfChains( arcs );
	chain->setMaxChainElements( segments + 1 );
	chain->setUseTextureCoords( true );
	chain->setUseVertexColours( true );
	// Like RibbonTrail: U runs across the strip, which is what the energy trail shader expects.
	chain->setTextureCoordDirection( Ogre::BillboardChain::TCD_V );
	chain->setMaterialName( material != NULL ? material->getValue() : "Tumbu/EnergyTrail/Lightning" );
	chain->setCastShadows( false );
	holder = sceneMgr->getRootSceneNode()->createChildSceneNode();
	holder->attachObject( chain );
}
//-------------------------------------------------------------------------------------
EffectLightningLayer::~EffectLightningLayer(void){
	Ogre::SceneManager* sceneMgr = effect->getManager()->getSceneManager();
	sceneMgr->destroyBillboardChain( chain );
	sceneMgr->destroySceneNode( holder );
}
//-------------------------------------------------------------------------------------
bool EffectLightningLayer::update( Ogre::Real age, Ogre::Real time ){
	if( stopped ){
		return false;
	}
	timer -= time;
	if( timer <= 0 ){
		timer = interval * Ogre::Math::RangeRandom( 0.7f, 1.3f );
		rebuild();
	}
	return true;
}
//-------------------------------------------------------------------------------------
static Ogre::Vector3 randomDirection(void){
	Ogre::Real z = Ogre::Math::RangeRandom( -1, 1 );
	Ogre::Radian a( Ogre::Math::RangeRandom( 0, Ogre::Math::TWO_PI ) );
	Ogre::Real r = Ogre::Math::Sqrt( std::max<Ogre::Real>( 0, 1 - z * z ) );
	return Ogre::Vector3( r * Ogre::Math::Cos( a ), z, r * Ogre::Math::Sin( a ) );
}
//-------------------------------------------------------------------------------------
void EffectLightningLayer::rebuild(void){
	Ogre::Vector3 centre = effect->getPosition();
	Ogre::Real strength = Ogre::Math::saturate( effect->getIntensity() );
	Ogre::Real r = radius * effect->getScale();
	for( int a = 0; a < arcs; a++ ){
		chain->clearChain( a );
		// Fewer arcs while the attack is weak.
		if( Ogre::Math::UnitRandom() > chance * strength ){
			continue;
		}
		Ogre::Vector3 from = randomDirection();
		Ogre::Vector3 to = randomDirection();
		bool leaping = Ogre::Math::UnitRandom() < leap;
		if( leaping ){
			to = ( from + to * 0.5f ).normalisedCopy();	// a bolt jumping off the surface
		}else if( to.dotProduct( from ) < 0.1f ){
			to = ( from + to ).normalisedCopy();	// an arc across a part of the ball, not through it
		}
		Ogre::Real outer = leaping ? 2.2f : 1.0f;
		for( int s = 0; s <= segments; s++ ){
			Ogre::Real t = (Ogre::Real)s / segments;
			Ogre::Vector3 direction = ( from * ( 1 - t ) + to * t ).normalisedCopy();
			Ogre::Real height = leaping ? 1 + ( outer - 1 ) * t : 1 + 0.12f * Ogre::Math::Sin( t * Ogre::Math::PI );
			Ogre::Vector3 position = centre + direction * r * height;
			if( s > 0 && s < segments ){
				position += randomDirection() * r * 0.18f;	// jagged
			}
			// Thin at both ends; the whole arc dims with the attack's strength.
			Ogre::Real taper = Ogre::Math::Sin( Ogre::Math::PI * std::max<Ogre::Real>( t, 0.08f ) );
			Ogre::ColourValue c = colour;
			c.a = 0.4f + 0.6f * strength;
			chain->addChainElement( a, Ogre::BillboardChain::Element( position, width * ( 0.4f + 0.6f * taper ), t, c, Ogre::Quaternion::IDENTITY ) );
		}
	}
}
//-------------------------------------------------------------------------------------
void EffectLightningLayer::stop(void){
	stopped = true;
}
//-------------------------------------------------------------------------------------
