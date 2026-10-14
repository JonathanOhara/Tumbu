#ifndef __ParticleFader_h_
#define __ParticleFader_h_

#include <Ogre.h>
#include <OgreParticleAffector.h>
#include <OgreParticleAffectorFactory.h>
#include <map>

/**
 * The particle affector "TumbuFader": ColourFader's job (red / green / blue / alpha change per second), but frame-rate
 * independent. Ogre 14 stores a particle's colour as bytes, and ColourFader adds rate x frame time and truncates every
 * frame, so a particle loses at least 1/255 per frame: at 800 fps nothing fades slower than about 3 alpha per second,
 * at 144 fps 0.56 (docs/OGRE_BUGS.md). TumbuFader keeps each particle's colour at birth and sets
 * colour = birth colour + rate x age (clamped) every frame, so the fade is the same at any frame rate.
 * Registered once (BaseApplication::locateResources, before the particle scripts are parsed) with install().
 */
class ParticleFader: public Ogre::ParticleAffector{
public:
	ParticleFader( Ogre::ParticleSystem* system );

	void _initParticle( Ogre::Particle* particle ) override;
	void _affectParticles( Ogre::ParticleSystem* system, Ogre::Real timeElapsed ) override;

	/// Registers the "TumbuFader" affector type with Ogre's particle system manager (once).
	static void install(void);

	Ogre::ColourValue rate;	// change per second

private:
	class CmdComponent: public Ogre::ParamCommand{
	public:
		CmdComponent( int _index ): index( _index ){}
		Ogre::String doGet( const void* target ) const override;
		void doSet( void* target, const Ogre::String &value ) override;
	private:
		int index;
	};
	static CmdComponent cmdRed, cmdGreen, cmdBlue, cmdAlpha;

	// the colour each live particle was born with (the pool reuses particles; _initParticle overwrites)
	std::map<Ogre::Particle*, Ogre::ColourValue> birth;
};

#endif // #ifndef __ParticleFader_h_
