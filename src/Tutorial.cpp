#include "Tutorial.h"
#include "GUI.h"
#include "TUMBU.h"
//-------------------------------------------------------------------------------------
Tutorial::Tutorial(Character *_hero, CharacterEnemy *_enemy){
	hero = _hero;
	enemy = _enemy;
	type = NONE;
	correctMove = false;

	TUMBU::getInstance()->getAIManager()->active = false;
}
//-------------------------------------------------------------------------------------
Tutorial::~Tutorial(void){
	TUMBU::getInstance()->getAIManager()->active = true;
}
//-------------------------------------------------------------------------------------
bool Tutorial::frameRenderingQueued(const Ogre::FrameEvent &evt){
	if( correctMove ){
		delay -= evt.timeSinceLastFrame;

		if( delay <= 0){

			switch( type ){
			case WALKING:
				runningTutorial();
				break;
			case RUNNING:
				rotateCameraTutorial();
				break;
			case ROTATING_CAMERA:
				kickTutorial();
				break;
			case KICKING:
				punchTutorial();
				break;
			case PUNCHING:
				prepareJynTutorial();
				break;
			case JYN_PREPARE:
				if( hero->jyn->canConcentrate() ){
					concentrateJynTutorial();
				}
				break;
			case JYN_CONCENTRATE:
				if( hero->jyn->canAttack() ){
					attackJynTutorial();
				}
				break;
			case JYN_ATTACK:
				defenseTutorial();
				break;
			case DEFENSE:
				menuTutorial();
				break;
			case MENU:
				finishTutorial();
				break;
			}
		}
	}

	return true;
}
//-------------------------------------------------------------------------------------
bool Tutorial::keyPressed( const OIS::KeyEvent &arg ){
	return true;
}
//-------------------------------------------------------------------------------------
bool Tutorial::keyReleased( const OIS::KeyEvent &arg ){
	switch( arg.key ){
	case OIS::KC_W:
		if( type == WALKING ){
			delay = 1;
			correctMove = true;
		}
		break;
	case OIS::KC_A:
		if( type == WALKING ){
			delay = 1;
			correctMove = true;
		}
		break;
	case OIS::KC_S:
		if( type == WALKING ){
			delay = 1;
			correctMove = true;
		}
		break;
	case OIS::KC_D:
		if( type == WALKING ){
			delay = 1;
			correctMove = true;
		}
		break;
	case OIS::KC_LSHIFT:
		if( type == RUNNING ){
			if( !hero->mKeyDirection.isZeroLength() ){
				delay = 1;
				correctMove = true;
			}
		}
		break;
	case OIS::KC_Q:
		if( type == ROTATING_CAMERA ){
			delay = 1;
			correctMove = true;
		}
		break;
	case OIS::KC_E:
		if( type == ROTATING_CAMERA ){
			delay = 1;
			correctMove = true;
		}
		break;

	case OIS::KC_U:
		if( type == KICKING ){
			if( hero->kick->isAttacking() ){
				delay = 1;
				correctMove = true;
			}
		}
		break;
	case OIS::KC_1:
		if( type == KICKING ){
			if( hero->kick->isAttacking() ){
				delay = 1;
				correctMove = true;
			}
		}
		break;

	case OIS::KC_O:
		if( type == PUNCHING ){
			if( hero->punch->isAttacking() ){
				delay = 1;
				correctMove = true;
			}
		}
		break;
	case OIS::KC_2:
		if( type == PUNCHING ){
			if( hero->punch->isAttacking() ){
				delay = 1;
				correctMove = true;
			}
		}
		break;

	case OIS::KC_I:
		if( type == JYN_PREPARE){
			delay = 0.1f;
			correctMove = true;
		}else if( type == JYN_CONCENTRATE ){
			delay = 0.5f;
			correctMove = true;
		}else if( type == JYN_ATTACK ){
			delay = 1.0f;
			correctMove = true;
		}
		break;
	case OIS::KC_3:
		if( type == JYN_PREPARE){
			delay = 0.1f;
			correctMove = true;
		}else if( type == JYN_CONCENTRATE ){
			delay = 0.5f;
			correctMove = true;
		}else if( type == JYN_ATTACK ){
			delay = 1.0f;
			correctMove = true;
		}
		break;

	case OIS::KC_P:
		if( type == DEFENSE ){
			if( !hero->isAttacking ){
				delay = 0.5f;
				correctMove = true;
			}
		}
		break;
	case OIS::KC_LCONTROL:
		if( type == DEFENSE ){
			if( !hero->isAttacking ){
				delay = 0.5f;
				correctMove = true;
			}
		}
		break;

	case OIS::KC_ESCAPE:
		if( type == MENU ){
			if( !GUI::getInstance()->isOptionOn() ){
				delay = 0.5f;
				correctMove = true;
			}
		}
		break;
	}

	hero->addHpInPercent( 100 );
	hero->addApInPercent( 100 );

	enemy->addHpInPercent( 100 );
	enemy->addApInPercent( 100 );

	return true;
}
//-------------------------------------------------------------------------------------
bool Tutorial::povMoved( const OIS::JoyStickEvent &e, int pov ) {
	int direction = e.state.mPOV[pov].direction;

	if( TUMBU::getInstance()->isPlaying() ){
		if( type == WALKING ){
			delay = 1;
			correctMove = true;
		}
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool Tutorial::axisMoved( const OIS::JoyStickEvent &e, int axis ) {
	int abs = e.state.mAxes[axis].abs;

	if( TUMBU::getInstance()->isPlaying() ){
		if( axis == 0 || axis == 1 ){
			if( type == WALKING ){
				delay = 1;
				correctMove = true;
			}
		}
	}	

	return true;
}
//-------------------------------------------------------------------------------------
bool Tutorial::sliderMoved( const OIS::JoyStickEvent &e, int sliderID ) {
	return true;
}
//------------------------------------------------------------------------------------- 
bool Tutorial::buttonPressed( const OIS::JoyStickEvent &e, int button ) {
	if( TUMBU::getInstance()->isPlaying() ){

	}
	return true;
}
//-------------------------------------------------------------------------------------
bool Tutorial::buttonReleased( const OIS::JoyStickEvent &e, int button ) {
	if( TUMBU::getInstance()->isPlaying() ){
		switch( button ){
		case 0:
			if( type == JYN_PREPARE){
				delay = 0.1f;
				correctMove = true;
			}else if( type == JYN_CONCENTRATE ){
				delay = 0.5f;
				correctMove = true;
			}else if( type == JYN_ATTACK ){
				delay = 1.0f;
				correctMove = true;
			}
			break;
		case 1:
			if( type == PUNCHING ){
				if( hero->punch->isAttacking() ){
					delay = 1;
					correctMove = true;
				}
			}
			break;
		case 2:
			if( type == KICKING ){
				if( hero->kick->isAttacking() ){
					delay = 1;
					correctMove = true;
				}
			}
			break;
		case 4:
			if( type == RUNNING ){
				if( !hero->mKeyDirection.isZeroLength() ){
					delay = 1;
					correctMove = true;
				}
			}
		case 5:
			if( type == DEFENSE ){
				if( !hero->isAttacking ){
					delay = 0.5f;
					correctMove = true;
				}
			}
			break;
		case 6:
			if( type == ROTATING_CAMERA ){
				delay = 1;
				correctMove = true;
			}
			break;
		case 7:
			if( type == ROTATING_CAMERA ){
				delay = 1;
				correctMove = true;
			}
			break;
		case 9:
			if( type == MENU ){
				if( !GUI::getInstance()->isOptionOn() ){
					delay = 0.5f;
					correctMove = true;
				}
			}
			break;
		}
	}

	hero->addHpInPercent( 100 );
	hero->addApInPercent( 100 );

	enemy->addHpInPercent( 100 );
	enemy->addApInPercent( 100 );

	return true;
}
//-------------------------------------------------------------------------------------
bool Tutorial::mouseMoved( const OIS::MouseEvent &arg ){
	if( type == ROTATING_CAMERA && TUMBU::getInstance()->isPlaying() && !correctMove){
		if( arg.state.X.rel < 2 || arg.state.X.rel > 2 ) {
			delay = 1.0f;
			correctMove = true;
		}
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool Tutorial::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id ){
	return true;
}
//-------------------------------------------------------------------------------------
bool Tutorial::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id ){
	return true;
}
//-------------------------------------------------------------------------------------
void Tutorial::startTutorial(){
	
	GUI::getInstance()->addAlert("Welcome to the Tutorial", "Welcome to the tutorial mode\n \nFollow step-by-step to learn the main moves in the \ngame." );

	walkingTutorial();
}
//-------------------------------------------------------------------------------------
void Tutorial::walkingTutorial(){
	type = WALKING;
	correctMove = false;

	GUI::getInstance()->addAlert("Walk Tutorial", "Learning to walk\n \nPress <W> or <JoyStick Axis Up> to forward\nPress <A> or <JoyStick Axis Left> to left\nPress <S> or <JoyStick Axis Down> to down\nPress <D> or <JoyStick Axis Right> to right" );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::runningTutorial(){
	type = RUNNING;
	correctMove = false;

	GUI::getInstance()->addAlert("Running Tutorial", "Learning to run\n \nHold <Left Shift> or <JoyStick button 4>\nAnd walk." );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::rotateCameraTutorial(){
	type = ROTATING_CAMERA;
	correctMove = false;

	GUI::getInstance()->addAlert("Walk Tutorial", "Congratulations You did it!" );
	GUI::getInstance()->addAlert("Camera Tutorial", "Learning to Manage the camera (view)\n \nTo rotate the camera press: \n<Q> or <E> \n<JoyStick button 6> or <JoyStick button 7> \n<Move Mouse Left> or <Move Mouse Right>" );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::kickTutorial(){
	type = KICKING;
	correctMove = false;

	GUI::getInstance()->addAlert("Camera Tutorial", "Congratulations You did it!" );
	GUI::getInstance()->addAlert("Kick Tutorial", "Learning to use the Kick Skill\n \nPress <U> or <1> or <JoyStick button 2>" );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::punchTutorial(){
	type = PUNCHING;
	correctMove = false;

	GUI::getInstance()->addAlert("Kick Tutorial", "Congratulations You did it!" );
	GUI::getInstance()->addAlert("Punch Tutorial", "Learning to use the Punch Skill\n \nPress <O> or <2> or <JoyStick button 1>" );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::prepareJynTutorial(){
	type = JYN_PREPARE;
	correctMove = false;

	GUI::getInstance()->addAlert("Punch Tutorial", "Congratulations You did it!" );
	GUI::getInstance()->addAlert("Jyn Tutorial", "Learning to use the Jyn Skill\n \nPress <I> or <3> or <JoyStick button 0> to call \nenergy balls" );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::concentrateJynTutorial(){
	type = JYN_CONCENTRATE;
	correctMove = false;

	GUI::getInstance()->addAlert("Jyn Tutorial", "Congratulations You did it!" );
	GUI::getInstance()->addAlert("Jyn Tutorial", "Learning to use the Jyn Skill\n \nPress <I> or <3> or <JoyStick button 0> to \nconcentrate the energy balls" );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::attackJynTutorial(){
	type = JYN_ATTACK;
	correctMove = false;

	GUI::getInstance()->addAlert("Jyn Tutorial", "Congratulations You did it!" );
	GUI::getInstance()->addAlert("Jyn Tutorial", "Learning to use the Jyn Skill\n \nPress <I> or <3> or <JoyStick button 0> to attack" );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::defenseTutorial(){
	type = DEFENSE;
	correctMove = false;

	GUI::getInstance()->addAlert("Jyn Tutorial", "Congratulations You did it!" );
	GUI::getInstance()->addAlert("Defense Tutorial", "Learning to use defense\n \nHold <P> or <Left Control> or <JoyStick button 5>\n \nNote: In defend mode your AP (breath points) \nrecovers more fast" );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::menuTutorial(){
	type = MENU;
	correctMove = false;

	GUI::getInstance()->addAlert("Defense Tutorial", "Congratulations You did it!" );
	GUI::getInstance()->addAlert("MENU Tutorial", "Learning to use the Menu\n \nPress <ESC> or <JoyStick button 9>" );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::finishTutorial(){
	type = FINISH;

	GUI::getInstance()->addAlert("Finish Tutorial", "Congratulations You finish the tutorial!\n \nGet ready to start the real fight", &GUI::stopTutorialMode );

	correctMove = false;
	GUI::getInstance()->showNextDialog(true);
}