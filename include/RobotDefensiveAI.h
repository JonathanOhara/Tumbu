#ifndef __RobotDefensiveAI_h_
#define __RobotDefensiveAI_h_

#include <Ogre.h>
#include <OIS.h>
#include <math.h>

#include "Character.h"
#include "CharacterEnemy.h"
#include "RobotAI.h"

class RobotDefensiveAI: public RobotAI{
public:
	RobotDefensiveAI( Character* _target, CharacterEnemy* _enemy);
	virtual ~RobotDefensiveAI(void);
	
	void update(const Ogre::Real time);
	void simulateKeyPressed();
	void simulateKeyReleased();

	void makeAction();
	void makeMovement();

	void randomMovement();
protected:
private:
	bool 
		defense,
		movement;

	Ogre::SceneNode
		*hNode,
		*eNode;
};

#endif // #ifndef __RobotDefensiveAI_h_