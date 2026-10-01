#include "EnergyParticle.h"
//-------------------------------------------------------------------------------------
EnergyParticle::EnergyParticle( Ogre::Particle* pParticle ){
	particle = pParticle;
	stream = NULL;
}
//-------------------------------------------------------------------------------------
EnergyParticle::~EnergyParticle(void){
	particle->mTimeToLive = 0;
}
//-------------------------------------------------------------------------------------