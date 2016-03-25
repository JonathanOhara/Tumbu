#ifndef __SpecialJyn_h_
#define __SpecialJyn_h_

#define MAX_ITERATIONS 240

#include "SpecialInterface.h"

class Robot;

class SpecialJyn: public SpecialInterface{
public:
	SpecialJyn( Ogre::SceneManager* _sceneMgr, Ogre::SceneNode* _particleSystemNode, Robot* _speller ,OgreBulletDynamics::DynamicsWorld* _world, int _count, float _damage );
	virtual ~SpecialJyn(void);
	
	void update(const Ogre::Real time);

	OgreBulletDynamics::RigidBody *getOgreBulletRigidBody( const std::string& instanceName );
	void collision( CollisionDetectionListener *other );

	void concentrate();
	void attack(Ogre::Quaternion orientation);
	void hitScenario( Ogre::Vector3 position );
	void hit(Ogre::SceneNode* hittedNode);
	void toDelete();
	void clear();

	void createRandomParticles();
	
	void avaliarDesempenhoTodos();
	double avaliarDesempenho(const Ogre::Vector3 pos);

	void moverTodasParticulas(Ogre::Vector3 moveTarget);
	void startAttack(Ogre::Quaternion orientation);
	
	void clearParticleSystem();

	int times,
		particulaMaiorFitness,
		melhorParticula;

	float tamanhoMaiorParticula;
	double fitnessMedio;
protected:

private:
	void executaPSO(const Ogre::Real time);
	void executaComplementoPSO();

	double 
		timePSO,
		timeToResest,
		melhorFitness,
		everBestFitness;

	Ogre::Vector3 
		everBestPosition,
		targetVector;
};

#endif // #ifndef __SpecialJyn_h_