#include "ParticleFader.h"
#include <OgreParticle.h>
#include <OgreParticleSystemManager.h>

ParticleFader::CmdComponent ParticleFader::cmdRed( 0 );
ParticleFader::CmdComponent ParticleFader::cmdGreen( 1 );
ParticleFader::CmdComponent ParticleFader::cmdBlue( 2 );
ParticleFader::CmdComponent ParticleFader::cmdAlpha( 3 );
ParticleFader::CmdComponent ParticleFader::cmdFadeIn( 4 );
ParticleFader::CmdComponent ParticleFader::cmdFadeOut( 5 );
ParticleFader::CmdComponent ParticleFader::cmdPeak( 6 );

namespace{
	class ParticleFaderFactory: public Ogre::ParticleAffectorFactory{
	public:
		Ogre::String getName() const override { return "TumbuFader"; }
		Ogre::ParticleAffector* createAffector( Ogre::ParticleSystem* system ) override { return new ParticleFader( system ); }
		void destroyAffector( Ogre::ParticleAffector* affector ) override { delete affector; }
	};
	class ParticleRandomiserFactory: public Ogre::ParticleAffectorFactory{
	public:
		Ogre::String getName() const override { return "TumbuRandomiser"; }
		Ogre::ParticleAffector* createAffector( Ogre::ParticleSystem* system ) override { return new ParticleRandomiser( system ); }
		void destroyAffector( Ogre::ParticleAffector* affector ) override { delete affector; }
	};
	// live as long as the program: Ogre's particle system manager keeps pointers to them until the Root goes
	ParticleFaderFactory factory;
	ParticleRandomiserFactory randomiserFactory;
	bool installed = false;
}
//-------------------------------------------------------------------------------------
ParticleFader::ParticleFader( Ogre::ParticleSystem* system ): Ogre::ParticleAffector( system ){
	mType = "TumbuFader";
	rate = Ogre::ColourValue( 0, 0, 0, 0 );
	fadeIn = fadeOut = 0;
	peak = 1;
	if( createParamDictionary( "TumbuFader" ) ){
		Ogre::ParamDictionary* dict = getParamDictionary();
		dict->addParameter( Ogre::ParameterDef( "red", "Change of the red component per second.", Ogre::PT_REAL ), &cmdRed );
		dict->addParameter( Ogre::ParameterDef( "green", "Change of the green component per second.", Ogre::PT_REAL ), &cmdGreen );
		dict->addParameter( Ogre::ParameterDef( "blue", "Change of the blue component per second.", Ogre::PT_REAL ), &cmdBlue );
		dict->addParameter( Ogre::ParameterDef( "alpha", "Change of the alpha component per second.", Ogre::PT_REAL ), &cmdAlpha );
		dict->addParameter( Ogre::ParameterDef( "fade_in", "Seconds the alpha takes to rise from 0 to peak at birth.", Ogre::PT_REAL ), &cmdFadeIn );
		dict->addParameter( Ogre::ParameterDef( "fade_out", "Seconds the alpha takes to fall to 0 at the end of the life.", Ogre::PT_REAL ), &cmdFadeOut );
		dict->addParameter( Ogre::ParameterDef( "peak", "The alpha between fade_in and fade_out.", Ogre::PT_REAL ), &cmdPeak );
	}
}
//-------------------------------------------------------------------------------------
void ParticleFader::_initParticle( Ogre::Particle* particle ){
	// the emitter has set the particle's colour already
	birth[particle] = Ogre::ColourValue( (Ogre::uchar*)&particle->mColour );
}
//-------------------------------------------------------------------------------------
void ParticleFader::_affectParticles( Ogre::ParticleSystem* system, Ogre::Real timeElapsed ){
	for( Ogre::Particle* p : system->_getActiveParticles() ){
		std::map<Ogre::Particle*, Ogre::ColourValue>::const_iterator b = birth.find( p );
		if( b == birth.end() ){
			continue;
		}
		const Ogre::Real age = p->mTotalTimeToLive - p->mTimeToLive;
		Ogre::ColourValue c = b->second + rate * age;
		if( fadeIn > 0 || fadeOut > 0 ){
			const Ogre::Real in = fadeIn > 0 ? age / fadeIn : 1, out = fadeOut > 0 ? p->mTimeToLive / fadeOut : 1;
			c.a = peak * std::min<Ogre::Real>( 1, std::min( in, out ) );
		}
		p->mColour = c.saturateCopy().getAsBYTE();
	}
}
//-------------------------------------------------------------------------------------
void ParticleFader::install(void){
	if( !installed ){
		Ogre::ParticleSystemManager::getSingleton().addAffectorFactory( &factory );
		Ogre::ParticleSystemManager::getSingleton().addAffectorFactory( &randomiserFactory );
		installed = true;
	}
}
//-------------------------------------------------------------------------------------
Ogre::String ParticleFader::CmdComponent::doGet( const void* target ) const{
	const ParticleFader* f = static_cast<const ParticleFader*>( target );
	return Ogre::StringConverter::toString( index < 4 ? f->rate[index] : index == 4 ? f->fadeIn : index == 5 ? f->fadeOut : f->peak );
}
//-------------------------------------------------------------------------------------
void ParticleFader::CmdComponent::doSet( void* target, const Ogre::String &value ){
	ParticleFader* f = static_cast<ParticleFader*>( target );
	const Ogre::Real v = Ogre::StringConverter::parseReal( value );
	if( index < 4 ){
		f->rate[index] = v;
	}else if( index == 4 ){
		f->fadeIn = v;
	}else if( index == 5 ){
		f->fadeOut = v;
	}else{
		f->peak = v;
	}
}
//-------------------------------------------------------------------------------------
ParticleRandomiser::Cmd ParticleRandomiser::cmdRandomness( 0 );
ParticleRandomiser::Cmd ParticleRandomiser::cmdScope( 1 );
ParticleRandomiser::Cmd ParticleRandomiser::cmdKeepVelocity( 2 );
//-------------------------------------------------------------------------------------
ParticleRandomiser::ParticleRandomiser( Ogre::ParticleSystem* system ): Ogre::ParticleAffector( system ){
	mType = "TumbuRandomiser";
	randomness = 1;
	scope = 1;
	keepVelocity = false;
	if( createParamDictionary( "TumbuRandomiser" ) ){
		Ogre::ParamDictionary* dict = getParamDictionary();
		dict->addParameter( Ogre::ParameterDef( "randomness", "How far a nudge turns the direction (as DirectionRandomiser at 60 fps).", Ogre::PT_REAL ), &cmdRandomness );
		dict->addParameter( Ogre::ParameterDef( "scope", "The share of particles nudged each frame (0..1).", Ogre::PT_REAL ), &cmdScope );
		dict->addParameter( Ogre::ParameterDef( "keep_velocity", "Keep each particle's speed.", Ogre::PT_BOOL ), &cmdKeepVelocity );
	}
}
//-------------------------------------------------------------------------------------
void ParticleRandomiser::_affectParticles( Ogre::ParticleSystem* system, Ogre::Real timeElapsed ){
	// a random walk: the nudge grows with the square root of the frame time (equal to DirectionRandomiser's at 60 fps)
	const Ogre::Real nudge = randomness * std::sqrt( std::max<Ogre::Real>( timeElapsed, 0 ) / 60.0f );
	for( Ogre::Particle* p : system->_getActiveParticles() ){
		if( scope <= Ogre::Math::UnitRandom() || p->mDirection.isZeroLength() ){
			continue;
		}
		const Ogre::Real length = p->mDirection.length();
		p->mDirection += Ogre::Vector3( Ogre::Math::RangeRandom( -nudge, nudge ), Ogre::Math::RangeRandom( -nudge, nudge ),
			Ogre::Math::RangeRandom( -nudge, nudge ) );
		if( keepVelocity ){
			p->mDirection *= length / p->mDirection.length();
		}
	}
}
//-------------------------------------------------------------------------------------
Ogre::String ParticleRandomiser::Cmd::doGet( const void* target ) const{
	const ParticleRandomiser* r = static_cast<const ParticleRandomiser*>( target );
	return index == 0 ? Ogre::StringConverter::toString( r->randomness ) : index == 1 ? Ogre::StringConverter::toString( r->scope )
		: Ogre::StringConverter::toString( r->keepVelocity );
}
//-------------------------------------------------------------------------------------
void ParticleRandomiser::Cmd::doSet( void* target, const Ogre::String &value ){
	ParticleRandomiser* r = static_cast<ParticleRandomiser*>( target );
	if( index == 0 ){
		r->randomness = Ogre::StringConverter::parseReal( value );
	}else if( index == 1 ){
		r->scope = Ogre::StringConverter::parseReal( value );
	}else{
		r->keepVelocity = Ogre::StringConverter::parseBool( value );
	}
}
