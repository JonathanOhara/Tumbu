#ifndef __SpecialInterface_h_
#define __SpecialInterface_h_

#include <Ogre.h>
#include <math.h>

#include "Physics.h"


#include "EnergyParticle.h"
#include "CollisionDetectionListener.h"

class Robot;
class Effect;

#define NUMBER_OF_PARTICLES 10

#define AC1 0.5f
#define AC2	0.5f

#define PARTICLE_WIDTH 0.25f
#define PARTICLE_HEIGHT 0.25f
#define PARTICLE_COLOR Ogre::ColourValue(0.93f, 0.11f, 0.14f, 1)
#define PARTICLE_LIVE_TIME 600
#define PARTICLE_MATERIAL "PE/energy"

#define PARTICLE_LIGHT_DIFFUSE_COLOR Ogre::ColourValue::White
#define PARTICLE_LIGHT_SPECULAR_COLOR Ogre::ColourValue::White

// Energy orbs (effects.material): the ball fills 0.45 of the billboard's half size, so a billboard is ORB_SCALE x the ball's
// diameter and the halo fits around it.
#define ORB_SCALE 2.2f
#define ORB_MATERIAL "Tumbu/EnergyOrb"
#define JYN_ORB_MATERIAL "Tumbu/EnergyOrb/Jyn"
// The kick's crescent wave (oriented along its flight): width x height of its billboard.
#define CRESCENT_MATERIAL "Tumbu/EnergyCrescent"
#define CRESCENT_WIDTH 1.3f
#define CRESCENT_HEIGHT 0.75f

class SpecialInterface: public CollisionDetectionListener{
public:
	SpecialInterface(void);
	virtual ~SpecialInterface(void);

	virtual void update(const Ogre::Real time) = 0;

	Physics::RigidBody *getOgreBulletRigidBody( const std::string& instanceName ) = 0;
	void collision( CollisionDetectionListener *other ) = 0;

	virtual void concentrate() = 0;
	virtual void attack(Ogre::Quaternion orientation) = 0;
	virtual void hitScenario( Ogre::Vector3 position ) = 0;
	virtual void hit(Ogre::SceneNode* hittedNode) = 0;
	virtual void toDelete() = 0;
	virtual void clear() = 0;
	
	Robot* getTarget();
	void setTarget( Robot* _robotTarget );

	/// Colour of this attack's energy: "kiColour" of the skill in skills.object, or else the speller's (its head's eyes).
	Ogre::ColourValue getKiColour( const Ogre::String &skillName );
	/// Sizes a particle as an energy orb whose ball is diameter wide (0 hides it).
	static void setOrbSize( Ogre::Particle* particle, Ogre::Real diameter );
	/// Particle colour of an orb: the ki colour, and a random seed in alpha for the shader.
	static Ogre::RGBA orbColour( const Ogre::ColourValue &ki );

	/// Starts the effect that travels with this attack's ball (light, trail, sparks: effects.object); replaces one
	/// already running.
	void startBallEffect( const Ogre::String &effectName, const Ogre::Vector3 &position, const Ogre::ColourValue &ki );
	/// Moves the ball effect (and sets its strength, 0..1).
	void updateBallEffect( const Ogre::Vector3 &position, Ogre::Real intensity = 1 );
	/// Lets the ball effect go: its light fades and its trail dies out on their own.
	void releaseBallEffect(void);

	Robot* getSpeller();
	void setSpeller( Robot* _robotSpeller );

	enum SpecialStatus { NONE, CONCENTRATING, CONCENTRATED, ATTACKING, HITTED, FADEOUT, FINISHED, TO_DELETE };

	SpecialStatus getSpecialStatus();
	void setSpecialStatus( SpecialStatus _specialStatus );

	std::list<Physics::RigidBody*> specialRigidBodyList;
	std::vector<EnergyParticle*> particleList;

	float damage;
protected:
	Ogre::SceneManager *sceneMgr;
	Physics::DynamicsWorld *world;
	Physics::BoxCollisionShape	*specialShape;

	Robot
		*robotSpeller,
		*robotTarget;
	
	Ogre::ParticleSystem
		*explosionParticleSystem,
		*particleSystem;
	
	Ogre::SceneNode 
		*specialRigidNode,
		*particleSystemNode;

	Ogre::Light *specialLight;

	SpecialStatus specialStatus;
	
	int count;
	/// The held effect following the ball (EffectsManager), or NULL.
	Effect* ballEffect;
private:
};

#endif // #ifndef __SpecialInterface_h_