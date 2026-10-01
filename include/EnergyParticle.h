#ifndef __EnergyParticle_h_
#define __EnergyParticle_h_

#include <Ogre.h>
#include "GameObject.h"

class Effect;

class EnergyParticle {
public:
	EnergyParticle( Ogre::Particle* pParticle );
	virtual ~EnergyParticle(void);

	bool active;

	double bestFitness;
	double fitness;

	Ogre::Particle* particle;
	Ogre::Vector3 bestPosition;
	Ogre::Vector3 velocity;
	/// Its stream (trail) while it flies into the Genki Dama (EffectsManager, held), or NULL.
	Effect* stream;
	/// Homing flight into the Genki Dama (SpecialJyn, gather homing): start offset from the gathering point, and the
	/// seconds it waits, has flown and takes.
	Ogre::Vector3 startOffset;
	Ogre::Real flightDelay, flightAge, flightTime;
protected:
private:
};

#endif // #ifndef __EnergyParticle_h_