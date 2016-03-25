#ifndef __AIManager_h_
#define __AIManager_h_

#include <Ogre.h>
#include <OIS.h>
#include <math.h>

#include "CharacterEnemy.h"
#include "GameObject.h"
#include "RobotAI.h"
#include "RobotDefensiveAI.h"

class AIManager: public Ogre::FrameListener{
public:
	AIManager( );
	virtual ~AIManager(void);
	static AIManager* getInstance(void);
	
	void reset();

	bool frameRenderingQueued(const Ogre::FrameEvent& evt);

	void createDefensiveRobotAI( Character* _target, CharacterEnemy* _enemy );
	void destroyRobotAI( RobotAI* robotAI );

	bool active;
protected:
	
private:
	std::list<RobotAI*> aiList;

	/** Para os iterators*/
	RobotAI* _robotAI;
	std::list<RobotAI*>::iterator itAiList;
	std::list<RobotAI*>::iterator itAiListEnd;

	static AIManager* instance;
};

#endif // #ifndef __AIManager_h_