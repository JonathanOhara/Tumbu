#include "DevTest.h"
#include "TUMBU.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif

bool DevTest::autoplay = false;
bool DevTest::walkTest = false;
int DevTest::fpsCap = 0;
Ogre::Real DevTest::quitAfter = 0;
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
		}else if( Ogre::StringUtil::startsWith( arg, "-fpscap=" ) ){
			fpsCap = Ogre::StringConverter::parseInt( arg.substr( 8 ) );
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
	return autoplay || fpsCap > 0;
}
//-------------------------------------------------------------------------------------
DevTest::DevTest(void){
	stage		= WAITING_START_SCREEN;
	frames		= 0;
	playTime	= 0;
	nextLogTime	= 0;
	walking		= false;
	walkDone	= false;
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
