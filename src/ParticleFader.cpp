#include "ParticleFader.h"
#include <OgreParticle.h>
#include <OgreParticleSystemManager.h>

ParticleFader::CmdComponent ParticleFader::cmdRed( 0 );
ParticleFader::CmdComponent ParticleFader::cmdGreen( 1 );
ParticleFader::CmdComponent ParticleFader::cmdBlue( 2 );
ParticleFader::CmdComponent ParticleFader::cmdAlpha( 3 );

namespace{
	class ParticleFaderFactory: public Ogre::ParticleAffectorFactory{
	public:
		Ogre::String getName() const override { return "TumbuFader"; }
		Ogre::ParticleAffector* createAffector( Ogre::ParticleSystem* system ) override { return new ParticleFader( system ); }
		void destroyAffector( Ogre::ParticleAffector* affector ) override { delete affector; }
	};
	// lives as long as the program: Ogre's particle system manager keeps a pointer to it until the Root goes
	ParticleFaderFactory factory;
	bool installed = false;
}
//-------------------------------------------------------------------------------------
ParticleFader::ParticleFader( Ogre::ParticleSystem* system ): Ogre::ParticleAffector( system ){
	mType = "TumbuFader";
	rate = Ogre::ColourValue( 0, 0, 0, 0 );
	if( createParamDictionary( "TumbuFader" ) ){
		Ogre::ParamDictionary* dict = getParamDictionary();
		dict->addParameter( Ogre::ParameterDef( "red", "Change of the red component per second.", Ogre::PT_REAL ), &cmdRed );
		dict->addParameter( Ogre::ParameterDef( "green", "Change of the green component per second.", Ogre::PT_REAL ), &cmdGreen );
		dict->addParameter( Ogre::ParameterDef( "blue", "Change of the blue component per second.", Ogre::PT_REAL ), &cmdBlue );
		dict->addParameter( Ogre::ParameterDef( "alpha", "Change of the alpha component per second.", Ogre::PT_REAL ), &cmdAlpha );
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
		p->mColour = ( b->second + rate * age ).saturateCopy().getAsBYTE();
	}
}
//-------------------------------------------------------------------------------------
void ParticleFader::install(void){
	if( !installed ){
		Ogre::ParticleSystemManager::getSingleton().addAffectorFactory( &factory );
		installed = true;
	}
}
//-------------------------------------------------------------------------------------
Ogre::String ParticleFader::CmdComponent::doGet( const void* target ) const{
	return Ogre::StringConverter::toString( static_cast<const ParticleFader*>( target )->rate[index] );
}
//-------------------------------------------------------------------------------------
void ParticleFader::CmdComponent::doSet( void* target, const Ogre::String &value ){
	static_cast<ParticleFader*>( target )->rate[index] = Ogre::StringConverter::parseReal( value );
}
