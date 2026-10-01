#ifndef __SpecialJyn_h_
#define __SpecialJyn_h_

#define MAX_ITERATIONS 240

#include "SpecialInterface.h"

class Robot;
class Part;

class SpecialJyn: public SpecialInterface{
public:
	SpecialJyn( Ogre::SceneManager* _sceneMgr, Ogre::SceneNode* _particleSystemNode, Robot* _speller ,Physics::DynamicsWorld* _world, int _count, float _damage );
	virtual ~SpecialJyn(void);
	
	void update(const Ogre::Real time);

	Physics::RigidBody *getOgreBulletRigidBody( const std::string& instanceName );
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
	/// Where the ball gathers: above the speller, followed from the cast until the attack.
	Ogre::Vector3 getChargeAnchor(void);
	/// World position of a bone of a part's entity; false when the part or bone is missing.
	bool getBonePosition( Part* part, const char* boneName, Ogre::Vector3 &position );
	void startAttack(Ogre::Quaternion orientation);
	
	void clearParticleSystem();

	/// Streams of the swarm balls: follow them, and are let go when a ball merges into the Genki Dama.
	void updateStreams(void);
	void releaseStreams(void);

	int times,
		particulaMaiorFitness,
		melhorParticula;

	float tamanhoMaiorParticula;
	/// Size the ball shows: it grows towards tamanhoMaiorParticula a little at a time as the swarm balls arrive.
	float tamanhoVisivel;
	/// The PSO has converged; the special becomes CONCENTRATED once the ball has finished growing.
	bool convergiu;
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

	/// The thrown ball's wake and sparks (held), or NULL.
	Effect* throwEffect;
	/// The gathering (motes, dust, lightning) from the cast until the throw (held), or NULL.
	Effect* chargeEffect;
	void releaseChargeEffect(void);

	Ogre::Vector3 
		everBestPosition,
		targetVector;
};

#endif // #ifndef __SpecialJyn_h_