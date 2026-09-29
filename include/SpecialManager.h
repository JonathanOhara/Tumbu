#ifndef __SpecialManager_h_
#define __SpecialManager_h_

#include <Ogre.h>
#include <math.h>

#include "Physics.h"


#include "SpecialInterface.h"
#include "SpecialJyn.h"
#include "SpecialPunch.h"
#include "SpecialKick.h"

class SpecialManager{
public:
	SpecialManager( Ogre::SceneManager* _sceneMgr, Robot* _robotSpeller,  Physics::DynamicsWorld* _world );
	virtual ~SpecialManager(void);

	void update(const Ogre::Real time);
	void collision(btRigidBody *rigid, btRigidBody *rigid2);

	Robot* getTarget();
	void setTarget( Robot* _robotTarget );

	Robot* getSpeller();
	void setSpeller( Robot* _robotSpeller );

	void destroySpecial( SpecialInterface* special );

	/** MOVES */
	SpecialJyn* specialJyn( float _damage );
	SpecialPunch* specialPunch( float _damage );
	SpecialKick* specialKick( float _damage );

	Ogre::SceneNode 
		*specialNode;
protected:
private:
	std::list<SpecialInterface*> specialList;
	
	Physics::DynamicsWorld *world;
	
	Ogre::SceneManager *sceneMgr;

	Robot
		*robotSpeller,
		*robotTarget;

	int count;

	/** Para os iterators*/
	SpecialInterface* _special;
	std::list<SpecialInterface*>::iterator itSpecialList;
	std::list<SpecialInterface*>::iterator itSpecialListEnd;
};

#endif // #ifndef __SpecialManager_h_