#ifndef __Tutorial_h_
#define __Tutorial_h_

#include <Ogre.h>
#include <OIS.h>

#include <iostream>

#include "Character.h"
#include "CharacterEnemy.h"

using namespace std;

class Tutorial: public OIS::MouseListener, public OIS::KeyListener, public OIS::JoyStickListener, public Ogre::FrameListener{
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
	bool keyPressed( const OIS::KeyEvent &arg );
	bool keyReleased( const OIS::KeyEvent &arg );

	bool mouseMoved(const OIS::MouseEvent &arg);
	bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);

    bool povMoved( const OIS::JoyStickEvent &e, int pov );
    bool axisMoved( const OIS::JoyStickEvent &e, int axis );
    bool sliderMoved( const OIS::JoyStickEvent &e, int sliderID );
    bool buttonPressed( const OIS::JoyStickEvent &e, int button );
    bool buttonReleased( const OIS::JoyStickEvent &e, int button );

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