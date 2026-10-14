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
 * colour = birth colour + rate x age (clamped) every frame, so the fade is the same at any frame rate. With fade_in /
 * fade_out (seconds) the alpha instead rises from 0 to `peak` over the first fade_in seconds of a particle's life and
 * falls back to 0 over its last fade_out seconds (ColourFader2's job; the arena dust).
 * Registered once (BaseApplication::locateResources, before the particle scripts are parsed) with install().
 */
class ParticleFader: public Ogre::ParticleAffector{
public:
	ParticleFader( Ogre::ParticleSystem* system );

	void _initParticle( Ogre::Particle* particle ) override;
	void _affectParticles( Ogre::ParticleSystem* system, Ogre::Real timeElapsed ) override;

	/// Registers the "TumbuFader" and "TumbuRandomiser" affector types with Ogre's particle system manager (once).
	static void install(void);

	Ogre::ColourValue rate;	// change per second
	Ogre::Real fadeIn, fadeOut, peak;

private:
	class CmdComponent: public Ogre::ParamCommand{
	public:
		CmdComponent( int _index ): index( _index ){}	// 0..3 red green blue alpha, 4 fade_in, 5 fade_out, 6 peak
		Ogre::String doGet( const void* target ) const override;
		void doSet( void* target, const Ogre::String &value ) override;
	private:
		int index;
	};
	static CmdComponent cmdRed, cmdGreen, cmdBlue, cmdAlpha, cmdFadeIn, cmdFadeOut, cmdPeak;

	// the colour each live particle was born with (the pool reuses particles; _initParticle overwrites)
	std::map<Ogre::Particle*, Ogre::ColourValue> birth;
};

/**
 * The particle affector "TumbuRandomiser": DirectionRandomiser's job (each frame, with probability `scope`, nudge a
 * particle's direction by up to `randomness`; keep_velocity), frame-rate independent. Ogre's nudge is
 * randomness x frame time, a random walk whose spread per second grows with the square root of the frame time: the
 * dust wandered 3.7x less at 800 fps than at 60. This one nudges by randomness x sqrt(frame time / 60), the same as
 * Ogre's at 60 fps and the same spread per second at any frame rate.
 */
class ParticleRandomiser: public Ogre::ParticleAffector{
public:
	ParticleRandomiser( Ogre::ParticleSystem* system );
	void _affectParticles( Ogre::ParticleSystem* system, Ogre::Real timeElapsed ) override;

	Ogre::Real randomness, scope;
	bool keepVelocity;

private:
	class Cmd: public Ogre::ParamCommand{
	public:
		Cmd( int _index ): index( _index ){}	// 0 randomness, 1 scope, 2 keep_velocity
		Ogre::String doGet( const void* target ) const override;
		void doSet( void* target, const Ogre::String &value ) override;
	private:
		int index;
	};
	static Cmd cmdRandomness, cmdScope, cmdKeepVelocity;
};

#endif // #ifndef __ParticleFader_h_
