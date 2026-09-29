#ifndef __Tutorial_h_
#define __Tutorial_h_

#include <Ogre.h>

#include <iostream>

#include "Character.h"
#include "CharacterEnemy.h"

using namespace std;

class Tutorial: public OgreBites::InputListener, public Ogre::FrameListener{
public:
	Tutorial(Character *_hero, CharacterEnemy *_enemy);
    virtual ~Tutorial(void);
	
	void startTutorial();
	void walkingTutorial();
	void runningTutorial();
	void rotateCameraTutorial();
	void kickTutorial();
	void punchTutorial();
	void prepareJynTutorial();
	void concentrateJynTutorial();
	void attackJynTutorial();
	void defenseTutorial();
	void menuTutorial();
	void finishTutorial();

	bool frameRenderingQueued(const Ogre::FrameEvent &evt);
	bool keyPressed( const OgreBites::KeyboardEvent &arg );
	bool keyReleased( const OgreBites::KeyboardEvent &arg );

	bool mouseMoved(const OgreBites::MouseMotionEvent &arg);
	bool mousePressed(const OgreBites::MouseButtonEvent &arg);
	bool mouseReleased(const OgreBites::MouseButtonEvent &arg);

    bool hatMoved( const OgreBites::HatEvent &e );
    bool axisMoved( const OgreBites::AxisEvent &e );
    bool buttonPressed( const OgreBites::ButtonEvent &e );
    bool buttonReleased( const OgreBites::ButtonEvent &e );

	enum TutorialType { NONE, WALKING, RUNNING, ROTATING_CAMERA, KICKING, PUNCHING, JYN_PREPARE, JYN_CONCENTRATE, JYN_ATTACK, DEFENSE, MENU, FINISH };

	TutorialType type;
protected:

private:
	Ogre::Real delay;
	bool correctMove;
	Character *hero;
	CharacterEnemy *enemy;
};

#endif // #ifndef __Tutorial_h_