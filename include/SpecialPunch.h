#ifndef __SpecialPunch_h_
#define __SpecialPunch_h_

#include "SpecialInterface.h"
#include "EnergyParticle.h"

class SpecialPunch: public SpecialInterface{
public:
	SpecialPunch( Ogre::SceneManager* _sceneMgr, Ogre::SceneNode* _particleSystemNode, Robot* _speller ,OgreBulletDynamics::DynamicsWorld* _world, int _count, float _damage );
	virtual ~SpecialPunch(void);
	
	void update(const Ogre::Real time);

	OgreBulletDynamics::RigidBody *getOgreBulletRigidBody( const std::string& instanceName );
	void collision( CollisionDetectionListener *other );

	void concentrate();
	void attack(Ogre::Quaternion orientation);
	void hit(Ogre::SceneNode* hittedNode);
	void hitScenario( Ogre::Vector3 position );
	void toDelete();
	void clear();
	
protected:

private:
	double timeToResest;
};

#endif // #ifndef __SpecialPunch_h_