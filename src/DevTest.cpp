#include "DevTest.h"
#include "TUMBU.h"

#include <MyGUI.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif

bool DevTest::autoplay = false;
bool DevTest::walkTest = false;
bool DevTest::guiTour = false;
int DevTest::fpsCap = 0;
Ogre::Real DevTest::quitAfter = 0;
int DevTest::startHour = -1;
//-------------------------------------------------------------------------------------
void DevTest::parseCommandLine( const Ogre::String &commandLine ){
	Ogre::StringVector args = Ogre::StringUtil::split( commandLine, " \t" );
	for( size_t i = 0; i < args.size(); i++ ){
		Ogre::String arg = args[i];
		Ogre::StringUtil::toLowerCase( arg );

		if( arg == "-autoplay" ){
			autoplay = true;
		}else if( arg == "-walktest" ){
			walkTest = true;
		}else if( arg == "-guitour" ){
			guiTour = true;
		}else if( Ogre::StringUtil::startsWith( arg, "-fpscap=" ) ){
			fpsCap = Ogre::StringConverter::parseInt( arg.substr( 8 ) );
		}else if( Ogre::StringUtil::startsWith( arg, "-hour=" ) ){
			startHour = Ogre::StringConverter::parseInt( arg.substr( 6 ) ) % 24;
		}else if( Ogre::StringUtil::startsWith( arg, "-quitafter=" ) ){
			quitAfter = Ogre::StringConverter::parseReal( arg.substr( 11 ) );
		}
	}
	// A walk test or timed quit only makes sense inside a match.
	if( walkTest || quitAfter > 0 ){
		autoplay = true;
	}
}
//-------------------------------------------------------------------------------------
bool DevTest::isEnabled(void){
	return autoplay || guiTour || fpsCap > 0;
}
//-------------------------------------------------------------------------------------
DevTest::DevTest(void){
	stage		= WAITING_START_SCREEN;
	frames		= 0;
	playTime	= 0;
	nextLogTime	= 0;
	walking		= false;
	walkDone	= false;
	tourStep	= 0;
	tourTimer	= 0;
	frameTimer.reset();

	log( "enabled: autoplay=" + Ogre::StringConverter::toString( autoplay ) +
		" walktest=" + Ogre::StringConverter::toString( walkTest ) +
		" fpscap=" + Ogre::StringConverter::toString( fpsCap ) +
		" quitafter=" + Ogre::StringConverter::toString( quitAfter ) );
}
//-------------------------------------------------------------------------------------
DevTest::~DevTest(void){
}
//-------------------------------------------------------------------------------------
bool DevTest::frameStarted( const Ogre::FrameEvent &evt ){
	limitFrameRate();
	frames++;

	if( guiTour ){
		runGuiTour( evt );
		return true;
	}

	if( !autoplay ){
		return true;
	}

	TUMBU* tumbu = TUMBU::getInstance();

	switch( stage ){
	case WAITING_START_SCREEN:
		// Give the start screen a few frames to settle before starting the match.
		if( tumbu->getGameState() == TumbuEnums::START_SCREEN && frames > 30 ){
			log( "starting match" );
			stage = WAITING_MATCH;
			GUI::getInstance()->hideStartMenu();
			tumbu->initializeDemo();
		}
		break;

	case WAITING_MATCH:
		if( tumbu->getGameState() == TumbuEnums::IN_DIALOG ){
			// Dismiss one intro dialog per frame (conversations, part previews, tutorial question).
			GUI::getInstance()->showNextDialog( true );
		}else if( tumbu->getGameState() == TumbuEnums::PLAYING ){
			log( "match playing" );
			stage = PLAYING;
		}
		break;

	case PLAYING:
		playTime += evt.timeSinceLastFrame;

		if( walkTest && tumbu->getDemo() != NULL && tumbu->getDemo()->mainChar != NULL ){
			Character* hero = tumbu->getDemo()->mainChar;
			if( !walking && !walkDone && playTime >= 1.0f ){
				log( "hero: press forward" );
				hero->movePressed( Robot::UP );
				walking = true;
			}else if( walking && playTime >= 4.0f ){
				log( "hero: release forward" );
				hero->moveReleased( Robot::UP );
				walking = false;
				walkDone = true;
			}
		}

		if( playTime >= nextLogTime ){
			logPositions();
			nextLogTime += 0.5f;
		}

		if( quitAfter > 0 && playTime >= quitAfter ){
			Ogre::String shot = tumbu->workPath + "devtest.png";
			tumbu->mWindow->writeContentsToFile( shot );
			log( "screenshot saved to " + shot );
			log( "quitting after " + Ogre::StringConverter::toString( playTime ) + "s" );
			stage = FINISHED;
			tumbu->shutdown();
		}
		break;

	case FINISHED:
		break;
	}

	return true;
}
//-------------------------------------------------------------------------------------
void DevTest::logPositions(void){
	Demo* demo = TUMBU::getInstance()->getDemo();
	if( demo == NULL || demo->mainChar == NULL || demo->enemy == NULL ){
		return;
	}

	Ogre::Vector3 hero = demo->mainChar->robotNode->_getDerivedPosition();
	Ogre::Vector3 enemy = demo->enemy->robotNode->_getDerivedPosition();
	const Ogre::RenderTarget::FrameStats& stats = TUMBU::getInstance()->mWindow->getStatistics();

	log( "t=" + Ogre::StringConverter::toString( playTime, 3 ) +
		" fps=" + Ogre::StringConverter::toString( stats.lastFPS, 4 ) +
		" hero=" + Ogre::StringConverter::toString( hero ) +
		" enemy=" + Ogre::StringConverter::toString( enemy ) );
}
//-------------------------------------------------------------------------------------
void DevTest::limitFrameRate(void){
	if( fpsCap <= 0 ){
		return;
	}

	const unsigned long frameMicros = 1000000UL / fpsCap;
	unsigned long elapsed = frameTimer.getMicroseconds();
	while( elapsed < frameMicros ){
		unsigned long remainingMillis = ( frameMicros - elapsed ) / 1000;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		Sleep( remainingMillis > 1 ? remainingMillis - 1 : 0 );
#else
		usleep( remainingMillis > 1 ? ( remainingMillis - 1 ) * 1000 : 0 );
#endif
		elapsed = frameTimer.getMicroseconds();
	}
	frameTimer.reset();
}
//-------------------------------------------------------------------------------------
void DevTest::log( const Ogre::String &message ){
	Ogre::LogManager::getSingletonPtr()->logMessage( "[DEVTEST] " + message );
}
//-------------------------------------------------------------------------------------
void DevTest::runGuiTour( const Ogre::FrameEvent &evt ){
	TUMBU* tumbu = TUMBU::getInstance();
	TumbuEnums::GameState state = tumbu->getGameState();
	tourTimer += evt.timeSinceLastFrame;

	switch( tourStep ){
	case 0: // start screen
		if( state == TumbuEnums::START_SCREEN && frames > 30 && tourTimer > 1.0f ){
			tourStep++; tourTimer = 0;	// advance first: actions may render frames (re-entering here)
			screenshot( "start" );
			click( "OptionsButton" );
		}
		break;
	case 1: // options window
		if( tourTimer > 0.5f ){
			tourStep++; tourTimer = 0;
			screenshot( "options" );
			click( "OptionsCancelButton" );
			click( "PlayButton" );
		}
		break;
	case 2: // first conversation (the wait starts when the dialog is really on screen)
		if( state != TumbuEnums::IN_DIALOG ){
			tourTimer = 0;
		}else if( tourTimer > 0.5f ){
			screenshot( "conversation" );
			tourStep++; tourTimer = 0;
		}
		break;
	case 3: // walk through the intro dialogs, photographing the part reward and the tutorial question
		if( state == TumbuEnums::IN_DIALOG ){
			if( tourTimer > 0.6f ){
				if( isVisible( "ShowPartWindow" ) ){
					screenshot( "showpart" );
				}
				if( isVisible( "ConfirmWindow" ) ){
					screenshot( "confirm" );
					pressKey( TumbuInput::KEY_ESCAPE );	// no tutorial
				}else{
					pressKey( TumbuInput::KEY_SPACE );
				}
				tourTimer = 0;
			}
		}else if( state == TumbuEnums::PLAYING ){
			tourStep++; tourTimer = 0;
		}
		break;
	case 4: // battle HUD
		if( tourTimer > 2.0f ){
			GUI::getInstance()->addLog( Log::LEVEL_UP, "GUI tour: log message", 5 );
			tourStep++; tourTimer = 0;
			screenshot( "hud" );
			pressKey( TumbuInput::KEY_ESCAPE );
		}
		break;
	case 5: // pause menu: status
		if( tourTimer > 0.5f ){
			screenshot( "pause-status" );
			click( "InventoryTab" );
			tourStep++; tourTimer = 0;
		}
		break;
	case 6:
		if( tourTimer > 1.0f ){
			screenshot( "pause-inventory" );
			click( "SkillsTab" );
			tourStep++; tourTimer = 0;
		}
		break;
	case 7:
		if( tourTimer > 0.5f ){
			screenshot( "pause-skills" );
			click( "HelpTab" );
			tourStep++; tourTimer = 0;
		}
		break;
	case 8:
		if( tourTimer > 0.5f ){
			screenshot( "pause-help" );
			pressKey( TumbuInput::KEY_ESCAPE );
			tourStep++; tourTimer = 0;
		}
		break;
	case 9:
		if( tourTimer > 0.5f ){
			log( "GUI tour finished" );
			tourStep++;
			tumbu->shutdown();
		}
		break;
	}
}
//-------------------------------------------------------------------------------------
void DevTest::screenshot( const Ogre::String &name ){
	Ogre::String file = TUMBU::getInstance()->workPath + "devtest-gui-" + name + ".png";
	TUMBU::getInstance()->mWindow->writeContentsToFile( file );
	log( "screenshot " + file );
}
//-------------------------------------------------------------------------------------
void DevTest::pressKey( int key ){
	OgreBites::KeyboardEvent evt;
	evt.type = OgreBites::KEYDOWN;
	evt.keysym.sym = key;
	evt.keysym.mod = 0;
	evt.repeat = 0;
	GUI::getInstance()->keyPressed( evt );
}
//-------------------------------------------------------------------------------------
void DevTest::click( const Ogre::String &buttonName ){
	MyGUI::Button *button = MyGUI::Gui::getInstance().findWidget<MyGUI::Button>( buttonName, false );
	if( button != NULL ){
		button->eventMouseButtonClick( button );
	}else{
		log( "click: no button " + buttonName );
	}
}
//-------------------------------------------------------------------------------------
bool DevTest::isVisible( const Ogre::String &widgetName ){
	MyGUI::Widget *widget = MyGUI::Gui::getInstance().findWidget<MyGUI::Widget>( widgetName, false );
	return widget != NULL && widget->getVisible();
}
//-------------------------------------------------------------------------------------
