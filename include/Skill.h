#ifndef __Skill_h_
#define __Skill_h_

#include <Ogre.h>
#include <OIS.h>
#include <math.h>

/** COLLISIONS */
#include <Utils/OgreBulletConverter.h>
#include <Debug/OgreBulletCollisionsDebugLines.h>
#include <Debug/OgreBulletCollisionsDebugShape.h>
#include <Debug/OgreBulletCollisionsDebugDrawer.h>
#include <Debug/OgreBulletCollisionsDebugContact.h>
#include <OgreBulletCollisionsShape.h>
#include <Shapes/OgreBulletCollisionsBoxShape.h>
#include <Shapes/OgreBulletCollisionsConeShape.h>
#include <Shapes/OgreBulletCollisionsSphereShape.h>
#include <Shapes/OgreBulletCollisionsCylinderShape.h>
#include <Shapes/OgreBulletCollisionsTriangleShape.h>
#include <Shapes/OgreBulletCollisionsTerrainShape.h>
#include <Shapes/OgreBulletCollisionsCylinderShape.h>
#include <Shapes/OgreBulletCollisionsCapsuleShape.h>
#include <Shapes/OgreBulletCollisionsStaticPlaneShape.h>
#include <Shapes/OgreBulletCollisionsCompoundShape.h>
#include <Shapes/OgreBulletCollisionsMultiSphereShape.h>
#include <Shapes/OgreBulletCollisionsConvexHullShape.h>
#include <Shapes/OgreBulletCollisionsMinkowskiSumShape.h>
#include <Shapes/OgreBulletCollisionsTrimeshShape.h>
#include <Shapes/OgreBulletCollisionsGImpactShape.h>
#include <Utils/OgreBulletCollisionsMeshToShapeConverter.h>

/** DYNAMICS */
#include <OgreBulletDynamicsRigidBody.h>
#include <OgreBulletDynamicsObjectState.h>
#include <OgreBulletDynamicsConstraint.h>
#include <Constraints/OgreBulletDynamics6DofConstraint.h>
#include <Constraints/OgreBulletDynamicsHingeConstraint.h>
#include <Constraints/OgreBulletDynamicsPoint2pointConstraint.h>
#include <Constraints/OgreBulletDynamicsConeTwistConstraint.h>
#include <Constraints/OgreBulletDynamicsRaycastVehicle.h>
#include <Prefab/OgreBulletDynamicsRagDoll.h>
#include <OgreBulletDynamicsWorld.h>

class Robot;

#include "SpecialManager.h"
#include "SpecialInterface.h"

using namespace std;

class Skill{
public:
	Skill( Ogre::String _skillID, Robot *_robot, bool _showLog );
	virtual ~Skill(void);
	
	void cast();
	bool canCast();

	void concentrate();
	bool canConcentrate();

	void attack(Ogre::Quaternion _orientation);
	bool canAttack();

	void toDelete();

	int getDamage();
	void setDamage( int _damage );

	int getEnergyBalls();
	void setEnergyBalls( int _energyBalls );

	int getAp();
	void setAp( int _ap );

	int getLevel();
	void setLevel( int _level );

	int getExperience();
	void setExperience( int _experience );

	int getExperienceNextLevel();
	void setExperienceNextLevel( int _experienceNextLevel );

	bool isAttacking();
	void setAttackFinished( bool _attackFinished );

	SpecialInterface *special;

	Ogre::String skillID;
protected:


private:
	void verifyLevelUp();
	SpecialManager* specialManager;

	bool
		showLog,
		attackFinished;

	Robot *robot;

	int 
		damage,
		energyBalls,
		ap,
		level,
		experience,
		experienceNextLevel;

	float experienceMultiplier;
};

#endif // #ifndef __Skill_h_