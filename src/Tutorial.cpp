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
bool Tutorial::keyPressed( const OgreBites::KeyboardEvent &arg ){
	return true;
}
//-------------------------------------------------------------------------------------
bool Tutorial::keyReleased( const OgreBites::KeyboardEvent &arg ){
	switch( arg.keysym.sym ){
	case 'w':
		if( type == WALKING ){
			delay = 1;
			correctMove = true;
		}
		break;
	case 'a':
		if( type == WALKING ){
			delay = 1;
			correctMove = true;
		}
		break;
	case 's':
		if( type == WALKING ){
			delay = 1;
			correctMove = true;
		}
		break;
	case 'd':
		if( type == WALKING ){
			delay = 1;
			correctMove = true;
		}
		break;
	case TumbuInput::KEY_LSHIFT:
		if( type == RUNNING ){
			if( !hero->mKeyDirection.isZeroLength() ){
				delay = 1;
				correctMove = true;
			}
		}
		break;
	case 'q':
		if( type == ROTATING_CAMERA ){
			delay = 1;
			correctMove = true;
		}
		break;
	case 'e':
		if( type == ROTATING_CAMERA ){
			delay = 1;
			correctMove = true;
		}
		break;

	case 'u':
		if( type == KICKING ){
			if( hero->kick->isAttacking() ){
				delay = 1;
				correctMove = true;
			}
		}
		break;
	case '1':
		if( type == KICKING ){
			if( hero->kick->isAttacking() ){
				delay = 1;
				correctMove = true;
			}
		}
		break;

	case 'o':
		if( type == PUNCHING ){
			if( hero->punch->isAttacking() ){
				delay = 1;
				correctMove = true;
			}
		}
		break;
	case '2':
		if( type == PUNCHING ){
			if( hero->punch->isAttacking() ){
				delay = 1;
				correctMove = true;
			}
		}
		break;

	case 'i':
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
	case '3':
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

	case 'p':
		if( type == DEFENSE ){
			if( !hero->isAttacking ){
				delay = 0.5f;
				correctMove = true;
			}
		}
		break;
	case TumbuInput::KEY_LCTRL:
		if( type == DEFENSE ){
			if( !hero->isAttacking ){
				delay = 0.5f;
				correctMove = true;
			}
		}
		break;

	case TumbuInput::KEY_ESCAPE:
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
bool Tutorial::hatMoved( const OgreBites::HatEvent &e ) {
	if( TUMBU::getInstance()->isPlaying() ){
		if( type == WALKING ){
			delay = 1;
			correctMove = true;
		}
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool Tutorial::axisMoved( const OgreBites::AxisEvent &e ) {
	if( TUMBU::getInstance()->isPlaying() ){
		if( ( e.axis == TumbuInput::AXIS_LEFTX || e.axis == TumbuInput::AXIS_LEFTY ) && Ogre::Math::Abs( e.value ) > TumbuInput::AXIS_DEADZONE ){
			if( type == WALKING ){
				delay = 1;
				correctMove = true;
			}
		}
		// Camera rotation is on the triggers and the right stick.
		if( ( e.axis == TumbuInput::AXIS_TRIGGERLEFT || e.axis == TumbuInput::AXIS_TRIGGERRIGHT || e.axis == TumbuInput::AXIS_RIGHTX ) && Ogre::Math::Abs( e.value ) > TumbuInput::AXIS_DEADZONE ){
			if( type == ROTATING_CAMERA ){
				delay = 1;
				correctMove = true;
			}
		}
	}	

	return true;
}
//-------------------------------------------------------------------------------------
bool Tutorial::buttonPressed( const OgreBites::ButtonEvent &e ) {
	if( TUMBU::getInstance()->isPlaying() ){

	}
	return true;
}
//-------------------------------------------------------------------------------------
bool Tutorial::buttonReleased( const OgreBites::ButtonEvent &e ) {
	if( TUMBU::getInstance()->isPlaying() ){
		switch( e.button ){
		case TumbuInput::PAD_SPECIAL:
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
		case TumbuInput::PAD_PUNCH:
			if( type == PUNCHING ){
				if( hero->punch->isAttacking() ){
					delay = 1;
					correctMove = true;
				}
			}
			break;
		case TumbuInput::PAD_KICK:
			if( type == KICKING ){
				if( hero->kick->isAttacking() ){
					delay = 1;
					correctMove = true;
				}
			}
			break;
		case TumbuInput::PAD_RUN:
			if( type == RUNNING ){
				if( !hero->mKeyDirection.isZeroLength() ){
					delay = 1;
					correctMove = true;
				}
			}
		case TumbuInput::PAD_GUARD:
			if( type == DEFENSE ){
				if( !hero->isAttacking ){
					delay = 0.5f;
					correctMove = true;
				}
			}
			break;
		case TumbuInput::PAD_MENU:
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
bool Tutorial::mouseMoved( const OgreBites::MouseMotionEvent &arg ){
	if( type == ROTATING_CAMERA && TUMBU::getInstance()->isPlaying() && !correctMove){
		if( arg.xrel < 2 || arg.xrel > 2 ) {
			delay = 1.0f;
			correctMove = true;
		}
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool Tutorial::mousePressed( const OgreBites::MouseButtonEvent &arg ){
	return true;
}
//-------------------------------------------------------------------------------------
bool Tutorial::mouseReleased( const OgreBites::MouseButtonEvent &arg ){
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

	GUI::getInstance()->addAlert("Walk Tutorial", "Learning to walk\n \nPress <W> or <Left stick Up> to forward\nPress <A> or <Left stick Left> to left\nPress <S> or <Left stick Down> to down\nPress <D> or <Left stick Right> to right" );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::runningTutorial(){
	type = RUNNING;
	correctMove = false;

	GUI::getInstance()->addAlert("Running Tutorial", "Learning to run\n \nHold <Left Shift> or <LB>\nAnd walk." );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::rotateCameraTutorial(){
	type = ROTATING_CAMERA;
	correctMove = false;

	GUI::getInstance()->addAlert("Walk Tutorial", "Congratulations You did it!" );
	GUI::getInstance()->addAlert("Camera Tutorial", "Learning to Manage the camera (view)\n \nTo rotate the camera press: \n<Q> or <E> \n<LT> or <RT> or <Right stick> \n<Move Mouse Left> or <Move Mouse Right>" );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::kickTutorial(){
	type = KICKING;
	correctMove = false;

	GUI::getInstance()->addAlert("Camera Tutorial", "Congratulations You did it!" );
	GUI::getInstance()->addAlert("Kick Tutorial", "Learning to use the Kick Skill\n \nPress <U> or <1> or <A>" );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::punchTutorial(){
	type = PUNCHING;
	correctMove = false;

	GUI::getInstance()->addAlert("Kick Tutorial", "Congratulations You did it!" );
	GUI::getInstance()->addAlert("Punch Tutorial", "Learning to use the Punch Skill\n \nPress <O> or <2> or <X>" );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::prepareJynTutorial(){
	type = JYN_PREPARE;
	correctMove = false;

	GUI::getInstance()->addAlert("Punch Tutorial", "Congratulations You did it!" );
	GUI::getInstance()->addAlert("Jyn Tutorial", "Learning to use the Jyn Skill\n \nPress <I> or <3> or <Y> to call \nenergy balls" );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::concentrateJynTutorial(){
	type = JYN_CONCENTRATE;
	correctMove = false;

	GUI::getInstance()->addAlert("Jyn Tutorial", "Congratulations You did it!" );
	GUI::getInstance()->addAlert("Jyn Tutorial", "Learning to use the Jyn Skill\n \nPress <I> or <3> or <Y> to \nconcentrate the energy balls" );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::attackJynTutorial(){
	type = JYN_ATTACK;
	correctMove = false;

	GUI::getInstance()->addAlert("Jyn Tutorial", "Congratulations You did it!" );
	GUI::getInstance()->addAlert("Jyn Tutorial", "Learning to use the Jyn Skill\n \nPress <I> or <3> or <Y> to attack" );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::defenseTutorial(){
	type = DEFENSE;
	correctMove = false;

	GUI::getInstance()->addAlert("Jyn Tutorial", "Congratulations You did it!" );
	GUI::getInstance()->addAlert("Defense Tutorial", "Learning to use defense\n \nHold <P> or <Left Control> or <RB>\n \nNote: In defend mode your AP (breath points) \nrecovers more fast" );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::menuTutorial(){
	type = MENU;
	correctMove = false;

	GUI::getInstance()->addAlert("Defense Tutorial", "Congratulations You did it!" );
	GUI::getInstance()->addAlert("MENU Tutorial", "Learning to use the Menu\n \nPress <ESC> or <Start>" );

	GUI::getInstance()->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Tutorial::finishTutorial(){
	type = FINISH;

	GUI::getInstance()->addAlert("Finish Tutorial", "Congratulations You finish the tutorial!\n \nGet ready to start the real fight", &GUI::stopTutorialMode );

	correctMove = false;
	GUI::getInstance()->showNextDialog(true);
}