#include "EnergyParticle.h"
//-------------------------------------------------------------------------------------
EnergyParticle::EnergyParticle( Ogre::Particle* pParticle ){
	particle = pParticle;
	stream = NULL;
	startOffset = Ogre::Vector3::ZERO;
	flightDelay = flightAge = 0;
	flightTime = 1;
}
//-------------------------------------------------------------------------------------
EnergyParticle::~EnergyParticle(void){
	particle->mTimeToLive = 0;
}
//-------------------------------------------------------------------------------------