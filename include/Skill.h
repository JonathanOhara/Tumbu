#ifndef __Skill_h_
#define __Skill_h_

#include <Ogre.h>
#include <math.h>

#include "Physics.h"


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