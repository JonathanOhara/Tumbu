#ifndef __EnergyParticle_h_
#define __EnergyParticle_h_

#include <Ogre.h>
#include <OIS.h>
#include "GameObject.h"

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
protected:
private:
};

#endif // #ifndef __EnergyParticle_h_