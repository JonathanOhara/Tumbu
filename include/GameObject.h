#ifndef __GameObject_h_
#define __GameObject_h_

#include <Ogre.h>
#include <math.h>

#include "Physics.h"
#include "Input.h"


#define CHAR_PARTS 5

using namespace std;

class GameObject{
public:
	GameObject(void);
    virtual ~GameObject(void);
	
	virtual void keyPressed(const OgreBites::KeyboardEvent &input) = 0;
	virtual void keyReleased(const OgreBites::KeyboardEvent &input) = 0;
	
	virtual void update(const Ogre::Real time) = 0;
private:
};

#endif // #ifndef __GameObject_h_