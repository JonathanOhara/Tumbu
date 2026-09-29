#include "EnergyParticle.h"
//-------------------------------------------------------------------------------------
EnergyParticle::EnergyParticle( Ogre::Particle* pParticle ){
	particle = pParticle;
}
//-------------------------------------------------------------------------------------
EnergyParticle::~EnergyParticle(void){
	particle->mTimeToLive = 0;
}
//-------------------------------------------------------------------------------------