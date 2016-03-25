#ifndef __RobotAI_h_
#define __RobotAI_h_

#include <Ogre.h>
#include <OIS.h>
#include <math.h>

#include "CharacterEnemy.h"

class RobotAI{
public:
	RobotAI( );
	virtual ~RobotAI(void);
	
	virtual void update(const Ogre::Real time) = 0;
	virtual void simulateKeyPressed() = 0;
	virtual void simulateKeyReleased() = 0;
	
	enum AIStatus { NONE, ACTIVE, TO_DELETE };

	AIStatus getAIStatus();
	void setAIStatus(AIStatus _aIStatus);

protected:
	CharacterEnemy* enemy;

	Ogre::Real timeSinceLastEvent;

	Ogre::Vector3 moveTarget;

private:

	AIStatus aIStatus;
};

#endif // #ifndef __RobotAI_h_