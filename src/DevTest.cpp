#ifdef _WIN32
#include <d3d11.h>	// before the game headers (their "using namespace std" clashes with the SDK's byte)
#include <d3d11sdklayers.h>
#endif
#include "DevTest.h"
#include "TUMBU.h"
#include "SpecialJyn.h"
#include "EffectsManager.h"
#include "Effect.h"
#include "AIManager.h"

#include <MyGUI.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <map>
#include <algorithm>
#include <fstream>
#include <ctime>
#else
#include <unistd.h>
#endif

bool DevTest::autoplay = false;
bool DevTest::walkTest = false;
bool DevTest::guiTour = false;
int DevTest::fpsCap = 0;
Ogre::Real DevTest::quitAfter = 0;
int DevTest::startHour = -1;
int DevTest::cycles = 0;
bool DevTest::measureAnims = false;
bool DevTest::mute = false;
int DevTest::faceShot = 0;
bool DevTest::jynWalk = false;
bool DevTest::noFx = false;
int DevTest::antiAliasing = -1;
int DevTest::sky = -1;
Ogre::Real DevTest::bench = 0;
bool DevTest::benchAI = false;
bool DevTest::benchChase = false;
Ogre::String DevTest::fxTest = "";
Ogre::String DevTest::heroSet = "";
Ogre::Real DevTest::fxTime = 0.3f;
Ogre::Real DevTest::fxDistance = 4.0f;
Ogre::Real DevTest::jynHit = 0;
bool DevTest::enemyThrows = false;
bool DevTest::fixedCamera = false;
bool DevTest::flyTest = false;
bool DevTest::swapTest = false;
Ogre::Vector3 DevTest::cameraEye = Ogre::Vector3::ZERO;
Ogre::Vector3 DevTest::cameraTarget = Ogre::Vector3::ZERO;
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
		}else if( arg == "-measureanims" ){
			measureAnims = true;
		}else if( arg == "-mute" ){
			mute = true;
		}else if( arg == "-nofx" ){
			noFx = true;
		}else if( Ogre::StringUtil::startsWith( arg, "-aa=" ) ){
			antiAliasing = Ogre::StringConverter::parseInt( arg.substr( 4 ) );
		}else if( Ogre::StringUtil::startsWith( arg, "-sky=" ) ){
			sky = Ogre::StringConverter::parseInt( arg.substr( 5 ) );
		}else if( arg == "-benchai" ){
			benchAI = true;
		}else if( arg == "-benchchase" ){
			benchChase = true;
		}else if( Ogre::StringUtil::startsWith( arg, "-bench=" ) ){
			bench = Ogre::StringConverter::parseReal( arg.substr( 7 ) );
		}else if( arg == "-flytest" ){
			flyTest = true;
		}else if( arg == "-swaptest" ){
			swapTest = true;
		}else if( arg == "-jynhit" || Ogre::StringUtil::startsWith( arg, "-jynhit=" ) ){
			// The enemy stands still in front of the hero, in Jyn's range, and the hero throws a Genki Dama at it.
			enemyThrows = arg == "-jynhit=enemy";
			jynHit = arg.size() > 8 && !enemyThrows ? Ogre::StringConverter::parseReal( arg.substr( 8 ) ) : 6.0f;
			if( fxTest.empty() ){
				fxTest = "special:jynthrow";
			}
		}else if( Ogre::StringUtil::startsWith( arg, "-fxtest=" ) ){
			fxTest = args[i].substr( 8 );	// effect names keep their case
		}else if( Ogre::StringUtil::startsWith( arg, "-fxdistance=" ) ){
			fxDistance = Ogre::StringConverter::parseReal( arg.substr( 12 ) );
		}else if( Ogre::StringUtil::startsWith( arg, "-fxtime=" ) ){
			fxTime = Ogre::StringConverter::parseReal( arg.substr( 8 ) );
		}else if( arg == "-jynwalk" ){
			jynWalk = true;
			walkTest = true;
		}else if( arg == "-faceshot" ){
			faceShot = 1;
		}else if( arg == "-faceshot=jyn" ){
			faceShot = 2;
		}else if( Ogre::StringUtil::startsWith( arg, "-camera=" ) ){
			// -camera=x,y,z,tx,ty,tz: the final screenshot looks from (x,y,z) at (tx,ty,tz), world units
			Ogre::StringVector v = Ogre::StringUtil::split( arg.substr( 8 ), "," );
			if( v.size() == 6 ){
				fixedCamera = true;
				cameraEye = Ogre::Vector3( Ogre::StringConverter::parseReal( v[0] ), Ogre::StringConverter::parseReal( v[1] ), Ogre::StringConverter::parseReal( v[2] ) );
				cameraTarget = Ogre::Vector3( Ogre::StringConverter::parseReal( v[3] ), Ogre::StringConverter::parseReal( v[4] ), Ogre::StringConverter::parseReal( v[5] ) );
			}
		}else if( Ogre::StringUtil::startsWith( arg, "-cycles=" ) ){
			cycles = Ogre::StringConverter::parseInt( arg.substr( 8 ) );
		}else if( Ogre::StringUtil::startsWith( arg, "-hero=" ) ){
			heroSet = arg.substr( 6 );
		}else if( Ogre::StringUtil::startsWith( arg, "-hour=" ) ){
			startHour = Ogre::StringConverter::parseInt( arg.substr( 6 ) ) % 24;
		}else if( Ogre::StringUtil::startsWith( arg, "-quitafter=" ) ){
			quitAfter = Ogre::StringConverter::parseReal( arg.substr( 11 ) );
		}
	}
	if( flyTest && quitAfter <= 0 ){
		quitAfter = 5;
	}
	// -bench: a 3 s warm-up, then the measured seconds; the standard robot view unless -camera is given.
	if( bench > 0 ){
		quitAfter = 3.0f + bench;
		if( !fixedCamera && !benchChase ){
			fixedCamera = true;
			cameraEye = Ogre::Vector3( 1.4f, 1.5f, 5.7f );
			cameraTarget = Ogre::Vector3( 0, 1.05f, 8.0f );
		}
	}
	// A walk test or timed quit only makes sense inside a match.
	if( walkTest || quitAfter > 0 ){
		autoplay = true;
	}
}
//-------------------------------------------------------------------------------------
bool DevTest::isEnabled(void){
	return autoplay || guiTour || cycles > 0 || measureAnims || fpsCap > 0;
}
//-------------------------------------------------------------------------------------
DevTest::DevTest(void){
	stage		= WAITING_START_SCREEN;
	frames		= 0;
	playTime	= 0;
	nextLogTime	= 0;
	walking		= false;
	walkDone	= false;
	faceJynPresses = 0;
	fxSpawned = false;
	fxPresses = 0;
	throwTime = -1;
	jynOverLogged = false;
	enemyHeld = false;
	enemyPresses = 0;
	heldEnemyAt = Ogre::Vector3::ZERO;
	fxEffect = NULL;
	fxSpot = Ogre::Vector3::ZERO;
	flyStep = 0;
	swapStep = 0;
	benchStarted = benchLogged = false;
	renderMicros = restMicros = 0;
	phaseFrames = 0;
	benchFrames = windowFrames = 0;
	windowMin = windowMax = 0;
	tourStep	= 0;
	tourTimer	= 0;
	cycle		= 0;
	kills		= 0;
	attacks		= 0;
	frameTimer.reset();

	log( "enabled: autoplay=" + Ogre::StringConverter::toString( autoplay ) +
		" walktest=" + Ogre::StringConverter::toString( walkTest ) +
		" fpscap=" + Ogre::StringConverter::toString( fpsCap ) +
		" quitafter=" + Ogre::StringConverter::toString( quitAfter ) +
		( fxTest.empty() ? "" : " fxtest=" + fxTest + " fxtime=" + Ogre::StringConverter::toString( fxTime ) + " fxdistance=" + Ogre::StringConverter::toString( fxDistance ) ) );
}
//-------------------------------------------------------------------------------------
DevTest::~DevTest(void){
}
//-------------------------------------------------------------------------------------
bool DevTest::frameStarted( const Ogre::FrameEvent &evt ){
	if( benchStarted && !benchLogged ){
		restMicros += phaseTimer.getMicroseconds();
		phaseTimer.reset();
	}
	limitFrameRate();
	frames++;

	if( measureAnims ){
		if( TUMBU::getInstance()->getGameState() == TumbuEnums::START_SCREEN && frames > 5 && frames < 1000000 ){
			frames = 1000000;
			measureWalkCycles();
			TUMBU::getInstance()->shutdown();
		}
		return true;
	}
	if( cycles > 0 ){
		runCycles( evt );
		return true;
	}
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
			{	// Robot parts skinned on the GPU (robots.program, RobotSkinning.h) or, as a fallback, on the CPU.
				int gpu = 0, cpu = 0;
				Ogre::String cpuNames;
				const Ogre::SceneManager::MovableObjectMap &entities = tumbu->mSceneMgr->getMovableObjects( "Entity" );
				for( Ogre::SceneManager::MovableObjectMap::const_iterator e = entities.begin(); e != entities.end(); e++ ){
					Ogre::Entity* entity = static_cast<Ogre::Entity*>( e->second );
					if( !entity->hasSkeleton() ){
						continue;
					}
					if( entity->isHardwareAnimationEnabled() ){
						gpu++;
					}else{
						cpu++;
						cpuNames += " " + entity->getName();
					}
				}
				log( "skinning: " + Ogre::StringConverter::toString( gpu ) + " entities on the GPU, " + Ogre::StringConverter::toString( cpu ) + " on the CPU" + cpuNames );
			}
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

		// -jynwalk: cast Jyn and concentrate it while the hero walks; the log shows whether the ball follows.
		if( jynWalk && faceJynPresses < 2 && playTime >= 1.3f + faceJynPresses * 0.5f ){
			log( faceJynPresses == 0 ? "jynwalk: Jyn cast" : "jynwalk: Jyn concentrate" );
			pressKey( 'i' );
			faceJynPresses++;
		}

		if( playTime >= nextLogTime ){
			logPositions();
			nextLogTime += 0.5f;
		}

		if( jynHit > 0 ){
			holdEnemy();
		}
		if( enemyThrows && enemyHeld && !fixedCamera && playTime >= quitAfter - 0.5f && tumbu->getDemo() != NULL ){
			// From the side, between the two robots.
			Ogre::Vector3 hero = tumbu->getDemo()->mainChar->robotNode->_getDerivedPosition();
			Ogre::Vector3 middle = ( hero + heldEnemyAt ) * 0.5f + Ogre::Vector3( 0, 1.2f, 0 );
			Ogre::Vector3 across = ( heldEnemyAt - hero ).crossProduct( Ogre::Vector3::UNIT_Y ).normalisedCopy();
			placeCamera( middle + across * fxDistance + Ogre::Vector3( 0, 0.2f * fxDistance, 0 ), middle );
		}
		if( !fxTest.empty() && quitAfter > 0 && !enemyThrows ){
			runFxTest();
		}
		if( faceShot > 0 && quitAfter > 0 ){
			faceCamera( evt );
		}
		if( flyTest ){
			runFlyTest();
		}
		if( swapTest ){
			runSwapTest();
		}
		if( bench > 0 ){
			runBench();
		}
		if( fixedCamera && quitAfter > 0 && playTime >= quitAfter - 0.5f ){
			placeCamera( cameraEye, cameraTarget );
		}

		if( quitAfter > 0 && playTime >= quitAfter ){
			Ogre::String shot = tumbu->workPath + "devtest.png";
			tumbu->mWindow->writeContentsToFile( shot );
			log( "screenshot saved to " + shot );
			if( faceShot > 0 && tumbu->getDemo() != NULL ){
				log( "faceshot: hero eye flare " + Ogre::StringConverter::toString( tumbu->getDemo()->mainChar->getEyeGlowBoost() ) + " aura " + Ogre::StringConverter::toString( tumbu->getDemo()->mainChar->getAuraLevel() ) );
			}
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
bool DevTest::frameRenderingQueued( const Ogre::FrameEvent &evt ){
	// -bench: from frameStarted to here Ogre updated the scene (animation, skinning, culling) and sent the draw calls.
	if( benchStarted && !benchLogged ){
		renderMicros += phaseTimer.getMicroseconds();
		phaseTimer.reset();
		phaseFrames++;
	}
	return true;
}
//-------------------------------------------------------------------------------------
void DevTest::faceCamera( const Ogre::FrameEvent &evt ){
	// -faceshot=jyn: cast Jyn 1.5 s before the shot and concentrate it 1 s before (eye flare, energy balls).
	if( faceShot == 2 && faceJynPresses < 2 && playTime >= quitAfter - 1.5f + faceJynPresses * 0.5f ){
		log( faceJynPresses == 0 ? "faceshot: Jyn cast" : "faceshot: Jyn concentrate" );
		pressKey( 'i' );
		faceJynPresses++;
	}
	// The last half second: the camera looks at the hero's face (the chase camera moves it back after each
	// frame is queued, so this is set again before every frame).
	Demo* demo = TUMBU::getInstance()->getDemo();
	if( playTime < quitAfter - 0.5f || demo == NULL || demo->mainChar == NULL ){
		return;
	}
	Ogre::SceneNode* head = demo->mainChar->headNode;
	Ogre::Vector3 face = head->_getDerivedPosition() + Ogre::Vector3( 0, 0.15f, 0 );
	Ogre::Vector3 forward = demo->mainChar->robotNode->_getDerivedOrientation() * Ogre::Vector3::UNIT_Z;
	forward.y = 0;
	forward.normalise();
	placeCamera( face + forward * 1.1f + Ogre::Vector3( 0.25f, 0.1f, 0 ), face );
}
//-------------------------------------------------------------------------------------
void DevTest::runFxTest(void){
	// -fxtest=<effect>: start the effect in front of the hero fxTime seconds before the screenshot, and look at the
	// hero and the effect from the side during the last half second (unless -camera or -faceshot place the camera).
	// -fxtest=special:jyn|punch|kick: the hero uses that attack instead (Jyn: cast, then concentrate 0.3 s later).
	Demo* demo = TUMBU::getInstance()->getDemo();
	if( demo == NULL || demo->mainChar == NULL || EffectsManager::getInstance() == NULL ){
		return;
	}
	Ogre::SceneNode* hero = demo->mainChar->robotNode;
	Ogre::Vector3 forward = hero->_getDerivedOrientation() * Ogre::Vector3::UNIT_Z;
	forward.y = 0;
	forward.normalise();
	Ogre::Vector3 front = hero->_getDerivedPosition() + forward * 2.0f + Ogre::Vector3( 0, 1.2f, 0 );

	bool special = Ogre::StringUtil::startsWith( fxTest, "special:", false );
	if( special ){
		Ogre::String attack = fxTest.substr( 8 );
		bool throwJyn = attack == "jynthrow";
		bool jyn = attack == "jyn" || throwJyn;
		SpecialJyn* charging = jyn && demo->mainChar->jyn != NULL ? dynamic_cast<SpecialJyn*>( demo->mainChar->jyn->special ) : NULL;
		// jynthrow: cast and concentrate at 1 s, throw as soon as the ball is ready; the shot is fxTime after the throw.
		if( throwJyn && fxPresses == 2 && charging != NULL && charging->getSpecialStatus() == SpecialInterface::CONCENTRATED ){
			log( "fxtest: hero jyn (throw)" );
			pressKey( 'i' );
			releaseKey( 'i' );
			fxPresses++;
			quitAfter = playTime + fxTime;
		}
		// After the throw: when the hero's Jyn is over (its arm comes down), how long after the throw.
		if( throwJyn && fxPresses == 3 && throwTime < 0 ){
			throwTime = playTime;
		}
		if( throwJyn && throwTime >= 0 && !jynOverLogged && demo->mainChar->jyn != NULL && !demo->mainChar->jyn->isAttacking() ){
			log( "fxtest: hero Jyn over (arm down) " + Ogre::StringConverter::toString( playTime - throwTime, 3 ) + " s after the throw" );
			jynOverLogged = true;
		}
		int presses = jyn ? 2 : 1;
		Ogre::Real start = throwJyn ? 1.0f : quitAfter - fxTime;
		if( fxPresses < presses && playTime >= start + fxPresses * 0.3f ){
			char key = jyn ? 'i' : ( attack == "kick" ? 'u' : 'o' );
			log( "fxtest: hero " + attack + ( fxPresses == 0 ? "" : " (concentrate)" ) );
			pressKey( key );
			releaseKey( key );
			fxPresses++;
		}
		// Jyn gathers above the head; punch and kick fly forward.
		fxSpot = jyn ? hero->_getDerivedPosition() + Ogre::Vector3( 0, 2.0f, 0 ) + forward * 0.6f : front;
		if( charging != NULL && !charging->particleList.empty() && charging->getSpecialStatus() <= SpecialInterface::ATTACKING ){
			fxSpot = charging->particleList[charging->melhorParticula]->particle->mPosition;	// the ball itself
		}
		fxSpawned = true;
	}else if( !fxSpawned && playTime >= quitAfter - fxTime ){
		fxSpot = front;
		// move:NAME is held, so it can be moved: it circles the spot (trails and sparks need motion). Otherwise the
		// effect plays once, as in a match.
		bool move = Ogre::StringUtil::startsWith( fxTest, "move:", false );
		Ogre::String name = move ? fxTest.substr( 5 ) : fxTest;
		Effect* effect = EffectsManager::getInstance()->spawn( name, fxSpot, Ogre::ColourValue( 1.0f, 0.3f, 0.15f ), move );
		fxEffect = move ? effect : NULL;
		log( "fxtest: " + name + ( effect != NULL ? " started at " + Ogre::StringConverter::toString( fxSpot ) : " is not defined" ) );
		fxSpawned = true;
	}
	if( fxEffect != NULL ){
		Ogre::Real angle = ( playTime - ( quitAfter - fxTime ) ) * 5.0f;
		fxEffect->setPosition( fxSpot + Ogre::Vector3( Ogre::Math::Cos( angle ), 0, Ogre::Math::Sin( angle ) ) * 0.8f );
	}
	if( !fixedCamera && faceShot == 0 && playTime >= quitAfter - 0.5f ){
		Ogre::Vector3 centre = fxSpawned ? fxSpot : front;
		if( jynHit > 0 && enemyHeld ){
			centre = heldEnemyAt + Ogre::Vector3( 0, 1.2f, 0 );	// -jynhit: the hero and the enemy
		}
		// From afar the shot frames the hero and the effect; close up (-fxdistance under 3) only the effect.
		Ogre::Vector3 middle = fxDistance < 3.0f ? centre : ( centre + hero->_getDerivedPosition() + Ogre::Vector3( 0, 1.0f, 0 ) ) * 0.5f;
		Ogre::Vector3 right = forward.crossProduct( Ogre::Vector3::UNIT_Y );
		placeCamera( middle + right * fxDistance + Ogre::Vector3( 0, 0.2f * fxDistance, 0 ) - forward * ( 0.125f * fxDistance ), middle );
	}
}
//-------------------------------------------------------------------------------------
void DevTest::holdEnemy(void){
	// -jynhit: no AI, and the enemy is held jynHit units in front of the hero, facing it (placed once).
	Demo* demo = TUMBU::getInstance()->getDemo();
	if( demo == NULL || demo->mainChar == NULL || demo->enemy == NULL || demo->enemy->charRigidBody == NULL ){
		return;
	}
	AIManager::getInstance()->active = false;
	btRigidBody* body = demo->enemy->charRigidBody->getBulletRigidBody();
	// Until the throw the enemy keeps to the spot in front of the hero (the hero may still turn); then it stays put.
	bool thrown = fxPresses > 2;
	if( !enemyHeld || !thrown ){
		Ogre::Vector3 hero = demo->mainChar->robotNode->_getDerivedPosition();
		Ogre::Vector3 forward = demo->mainChar->robotNode->_getDerivedOrientation() * Ogre::Vector3::UNIT_Z;
		forward.y = 0;
		forward.normalise();
		heldEnemyAt = hero + forward * jynHit;
		heldEnemyAt.y = body->getWorldTransform().getOrigin().y();
		demo->enemy->robotNode->setOrientation( Ogre::Vector3::UNIT_Z.getRotationTo( -forward ) );
		if( !enemyHeld )
			log( "jynhit: enemy held at " + Ogre::StringConverter::toString( heldEnemyAt ) + ", " + Ogre::StringConverter::toString( jynHit ) + " units in front of the hero" );
		enemyHeld = true;
	}
	btTransform transform = body->getWorldTransform();
	transform.setOrigin( btVector3( heldEnemyAt.x, transform.getOrigin().y(), heldEnemyAt.z ) );
	body->setWorldTransform( transform );
	body->setInterpolationWorldTransform( transform );
	body->setLinearVelocity( btVector3( 0, body->getLinearVelocity().y(), 0 ) );
	body->setAngularVelocity( btVector3( 0, 0, 0 ) );

	// -jynhit=enemy: the enemy charges a Genki Dama (cast, concentrate) and throws it at the hero when it is ready;
	// the screenshot is fxtime after its throw.
	if( enemyThrows && quitAfter > 0 ){
		CharacterEnemy* enemy = demo->enemy;
		SpecialJyn* charging = enemy->jyn != NULL ? dynamic_cast<SpecialJyn*>( enemy->jyn->special ) : NULL;
		if( enemyPresses < 2 && playTime >= 1.0f + enemyPresses * 0.3f ){
			log( enemyPresses == 0 ? "jynhit: enemy casts Jyn" : "jynhit: enemy concentrates" );
			enemy->movePressed( Robot::JYN );
			enemyPresses++;
		}else if( enemyPresses == 2 && charging != NULL && charging->getSpecialStatus() == SpecialInterface::CONCENTRATED ){
			log( "jynhit: enemy throws" );
			enemy->movePressed( Robot::JYN );
			enemyPresses++;
			quitAfter = playTime + fxTime;
		}
	}
}
//-------------------------------------------------------------------------------------
void DevTest::placeCamera( const Ogre::Vector3 &eye, const Ogre::Vector3 &target ){
	// Set before every frame: the chase camera moves the camera back after each frame is queued.
	Ogre::SceneNode* node = TUMBU::getInstance()->mCamera->getParentSceneNode();
	node->_setDerivedPosition( eye );
	// Level orientation looking at the target (Ogre cameras look down their -Z axis).
	Ogre::Vector3 back = ( eye - target ).normalisedCopy();
	Ogre::Vector3 right = Ogre::Vector3::UNIT_Y.crossProduct( back ).normalisedCopy();
	node->_setDerivedOrientation( Ogre::Quaternion( right, back.crossProduct( right ), back ) );
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

	// A charging Jyn: where its ball is, relative to the point it gathers at (it must stay there while the hero walks).
	SpecialJyn* jyn = demo->mainChar->jyn != NULL ? dynamic_cast<SpecialJyn*>( demo->mainChar->jyn->special ) : NULL;
	if( jyn != NULL && !jyn->particleList.empty() && jyn->getSpecialStatus() <= SpecialInterface::CONCENTRATED ){
		log( "jyn: status=" + Ogre::StringConverter::toString( (int)jyn->getSpecialStatus() ) +
			" size=" + Ogre::StringConverter::toString( jyn->tamanhoVisivel, 3 ) + "/" + Ogre::StringConverter::toString( jyn->tamanhoMaiorParticula, 3 ) +
			" hand-robot=" + Ogre::StringConverter::toString( jyn->getChargeAnchor() - demo->mainChar->robotNode->_getDerivedPosition() ) +
			" ball-anchor=" + Ogre::StringConverter::toString( jyn->particleList[jyn->melhorParticula]->particle->mPosition - jyn->getChargeAnchor() ) );
	}
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
	case 4: // battle HUD, with a Jyn charging (its red energy particles are in the shot)
		if( attacks == 0 && tourTimer > 1.0f ){
			attacks = 1;
			pressKey( 'i' );
		}else if( tourTimer > 2.0f ){
			attacks = 0;
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
	case 8: // pause menu Quit: back to the start screen
		if( tourTimer > 0.5f ){
			screenshot( "pause-help" );
			tourStep++; tourTimer = 0;
			click( "QuitTab" );
		}
		break;
	case 9: // start screen again: play a second match
		if( state == TumbuEnums::START_SCREEN && tourTimer > 1.0f ){
			tourStep++; tourTimer = 0;
			screenshot( "back-to-menu" );
			click( "PlayButton" );
		}
		break;
	case 10: // dismiss the intro dialogs again
		if( state == TumbuEnums::IN_DIALOG ){
			if( tourTimer > 0.3f ){
				pressKey( isVisible( "ConfirmWindow" ) ? TumbuInput::KEY_ESCAPE : TumbuInput::KEY_SPACE );
				tourTimer = 0;
			}
		}else if( state == TumbuEnums::PLAYING && tourTimer > 0.3f ){
			tourStep++; tourTimer = 0;
		}
		break;
	case 11:
		if( tourTimer > 1.5f ){
			tourStep++; tourTimer = 0;
			screenshot( "second-match" );
			pressKey( TumbuInput::KEY_ESCAPE );
		}
		break;
	case 12:
		if( tourTimer > 0.5f ){
			tourStep++; tourTimer = 0;
			click( "QuitTab" );
		}
		break;
	case 13: // leave through the start screen's Exit button
		if( state == TumbuEnums::START_SCREEN && tourTimer > 1.0f ){
			log( "GUI tour finished" );
			tourStep++;
			click( "ExitButton" );
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
// Simulated input goes through the same dispatch as SDL events (BaseApplication → every listener).
static OgreBites::InputListener* input(void){
	return TUMBU::getInstance();
}
//-------------------------------------------------------------------------------------
void DevTest::pressKey( int key ){
	OgreBites::KeyboardEvent evt;
	evt.type = OgreBites::KEYDOWN;
	evt.keysym.sym = key;
	evt.keysym.mod = 0;
	evt.repeat = 0;
	input()->keyPressed( evt );
}
//-------------------------------------------------------------------------------------
void DevTest::releaseKey( int key ){
	OgreBites::KeyboardEvent evt;
	evt.type = OgreBites::KEYUP;
	evt.keysym.sym = key;
	evt.keysym.mod = 0;
	evt.repeat = 0;
	input()->keyReleased( evt );
}
//-------------------------------------------------------------------------------------
void DevTest::runFlyTest(void){
	// -flytest: F, fly forward and up, Esc (back to the match, no pause menu), F again; the screenshot is taken
	// while flying. Steps are spread over the last seconds before -quitafter.
	TUMBU* tumbu = TUMBU::getInstance();
	const Ogre::Real start = quitAfter - 3.5f;
	const char* stepNames[] = { "F", "fly forward and up", "stop moving", "Esc", "F again" };
	const Ogre::Real stepTimes[] = { 0.0f, 0.2f, 1.7f, 2.2f, 2.8f };
	while( flyStep < 5 && playTime >= start + stepTimes[flyStep] ){
		switch( flyStep ){
		case 0: pressKey( 'f' ); break;
		case 1: pressKey( 'w' ); pressKey( TumbuInput::KEY_SPACE ); break;
		case 2: releaseKey( 'w' ); releaseKey( TumbuInput::KEY_SPACE ); break;
		case 3: pressKey( TumbuInput::KEY_ESCAPE ); break;
		case 4: pressKey( 'f' ); break;
		}
		log( Ogre::String( "flytest: " ) + stepNames[flyStep] );
		flyStep++;
	}
	// Report the state a few frames after each step.
	static int lastLogged = -1;
	if( flyStep > 0 && lastLogged != flyStep && playTime >= start + stepTimes[flyStep - 1] + 0.15f ){
		lastLogged = flyStep;
		Ogre::Vector3 p = tumbu->mCamera->getDerivedPosition();
		log( "flytest: state=" + Ogre::StringConverter::toString( (int) tumbu->getGameState() ) + " (PLAYING=" + Ogre::StringConverter::toString( (int) TumbuEnums::PLAYING ) + ", FLYING=" + Ogre::StringConverter::toString( (int) TumbuEnums::FLYING ) + ") camera=" + Ogre::StringConverter::toString( p ) + " pauseMenu=" + Ogre::StringConverter::toString( isVisible( "PauseMenu" ) ) );
	}
}
//-------------------------------------------------------------------------------------
void DevTest::runSwapTest(void){
	// -swaptest: charge Jyn until the aura shows, pause, re-select the head in the inventory (GUI::changePart unbuilds
	// and rebuilds every part, the aura shells included), resume, then charge Jyn again on the rebuilt parts.
	TUMBU* tumbu = TUMBU::getInstance();
	Demo* demo = tumbu->getDemo();
	if( demo == NULL || demo->mainChar == NULL ){
		return;
	}
	const char* stepNames[] = { "Jyn cast", "Jyn concentrate", "Esc (pause)", "Inventory", "re-select the head",
		"Esc (resume)", "Jyn cast again", "Jyn concentrate again" };
	const Ogre::Real stepTimes[] = { 1.0f, 1.3f, 3.5f, 3.9f, 4.4f, 5.0f, 5.6f, 5.9f };
	while( swapStep < 8 && playTime >= stepTimes[swapStep] ){
		switch( swapStep ){
		case 0: case 1: case 6: case 7: pressKey( 'i' ); releaseKey( 'i' ); break;
		case 2: case 5: pressKey( TumbuInput::KEY_ESCAPE ); break;
		case 3: click( "InventoryTab" ); break;
		case 4:{
			// The combo's own accept event, as when the player picks an entry.
			MyGUI::ComboBox* combo = MyGUI::Gui::getInstance().findWidget<MyGUI::ComboBox>( "HeadCombo", false );
			if( combo == NULL || combo->getItemCount() == 0 ){
				log( "swaptest: no head to pick" );
				break;
			}
			combo->setIndexSelected( 0 );
			combo->eventComboAccept( combo, 0 );
			break;
		}
		}
		log( Ogre::String( "swaptest: " ) + stepNames[swapStep] + " state=" + Ogre::StringConverter::toString( (int) tumbu->getGameState() )
			+ " aura=" + Ogre::StringConverter::toString( demo->mainChar->getAuraLevel() )
			+ " shell=" + Ogre::StringConverter::toString( demo->mainChar->head->auraEntity != NULL && demo->mainChar->head->auraEntity->getVisible() ) );
		swapStep++;
	}
}
//-------------------------------------------------------------------------------------
void DevTest::runBench(void){
	// -bench=S: the same frames every run, so two runs differ only by what was changed. No AI (the enemy and the hero
	// stand idle), the camera fixed from the first frame; after a 3 s warm-up the frames of S seconds are counted with
	// a wall-clock timer (not Ogre's smoothed fps), plus the slowest and fastest 1 s window.
	// -benchai keeps the AI (the enemy walks and attacks), -benchchase the game's chase camera, to find what costs.
	if( !benchAI ){
		AIManager::getInstance()->active = false;
	}
	if( !benchChase ){
		placeCamera( cameraEye, cameraTarget );
	}
	if( playTime < 3.0f ){
		return;
	}
	if( !benchStarted ){
		benchStarted = true;
		benchFrames = windowFrames = 0;
		benchTimer.reset();
		renderMicros = restMicros = 0;
		phaseFrames = 0;
		phaseTimer.reset();
		windowTimer.reset();
		return;
	}
	benchFrames++;
	windowFrames++;
	unsigned long windowMicros = windowTimer.getMicroseconds();
	if( windowMicros >= 1000000UL ){
		Ogre::Real fps = windowFrames * 1000000.0f / windowMicros;
		windowMin = ( windowMin == 0 || fps < windowMin ) ? fps : windowMin;
		windowMax = std::max( windowMax, fps );
		windowFrames = 0;
		windowTimer.reset();
	}
	if( !benchLogged && playTime >= quitAfter - 0.05f ){
		benchLogged = true;
		double seconds = benchTimer.getMicroseconds() / 1000000.0;
		TUMBU* tumbu = TUMBU::getInstance();
		log( "bench: fps=" + Ogre::StringConverter::toString( (Ogre::Real)( benchFrames / seconds ), 5 ) +
			" ms=" + Ogre::StringConverter::toString( (Ogre::Real)( seconds * 1000.0 / benchFrames ), 4 ) +
			" frames=" + Ogre::StringConverter::toString( benchFrames ) +
			" seconds=" + Ogre::StringConverter::toString( (Ogre::Real) seconds, 4 ) +
			" window_min=" + Ogre::StringConverter::toString( windowMin, 5 ) +
			" window_max=" + Ogre::StringConverter::toString( windowMax, 5 ) +
			" render_ms=" + Ogre::StringConverter::toString( (Ogre::Real)( phaseFrames > 0 ? renderMicros / phaseFrames / 1000.0 : 0 ), 4 ) +
			" rest_ms=" + Ogre::StringConverter::toString( (Ogre::Real)( phaseFrames > 0 ? restMicros / phaseFrames / 1000.0 : 0 ), 4 ) +
			" render=" + Ogre::StringUtil::replaceAll( tumbu->getRoot()->getRenderSystem()->getName(), " ", "_" ) +
			" size=" + Ogre::StringConverter::toString( tumbu->mWindow->getWidth() ) + "x" + Ogre::StringConverter::toString( tumbu->mWindow->getHeight() ) +
			" aa=" + Ogre::StringConverter::toString( tumbu->isAntiAliasingEnabled() ) +
			" sky=" + Ogre::StringConverter::toString( sky >= 0 ? sky : tumbu->getSkyQuality() ) +
			" shadows=" + Ogre::StringConverter::toString( tumbu->getShadowPreset() ) +
			// A hidden or minimised window is not rendered: the loop only spins (thousands of fps, no render time).
			( phaseFrames > 0 && renderMicros / phaseFrames < 20.0 ? " invalid=window-not-rendered" : "" ) +
			( benchAI ? " ai" : "" ) + ( benchChase ? " chase" : "" ) );
	}
}
//-------------------------------------------------------------------------------------
void DevTest::click( const Ogre::String &buttonName ){
	// A real click through the game's input dispatch (every listener, then MyGUI), so a button the mouse cannot
	// reach fails here too.
	MyGUI::Button *button = MyGUI::Gui::getInstance().findWidget<MyGUI::Button>( buttonName, false );
	if( button == NULL ){
		log( "click: no button " + buttonName );
		return;
	}
	MyGUI::IntCoord coord = button->getAbsoluteCoord();
	OgreBites::MouseMotionEvent move;
	move.type = OgreBites::MOUSEMOTION;
	move.x = coord.left + coord.width / 2;
	move.y = coord.top + coord.height / 2;
	move.xrel = move.yrel = 0;
	move.windowID = 0;
	input()->mouseMoved( move );
	if( MyGUI::InputManager::getInstance().getMouseFocusWidget() != button ){
		log( "click: the mouse cannot reach " + buttonName );
		return;
	}
	OgreBites::MouseButtonEvent press;
	press.type = OgreBites::MOUSEBUTTONDOWN;
	press.x = move.x;
	press.y = move.y;
	press.button = OgreBites::BUTTON_LEFT;
	press.clicks = 1;
	input()->mousePressed( press );
	OgreBites::MouseButtonEvent release = press;
	release.type = OgreBites::MOUSEBUTTONUP;
	input()->mouseReleased( release );
}
//-------------------------------------------------------------------------------------
bool DevTest::isVisible( const Ogre::String &widgetName ){
	MyGUI::Widget *widget = MyGUI::Gui::getInstance().findWidget<MyGUI::Widget>( widgetName, false );
	return widget != NULL && widget->getVisible();
}
//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------
static size_t countWidgets( MyGUI::Widget *widget ){
	size_t count = 1;
	for( size_t i = 0; i < widget->getChildCount(); i++ ){
		count += countWidgets( widget->getChildAt( i ) );
	}
	return count;
}
//-------------------------------------------------------------------------------------
static size_t countWidgets(void){
	size_t count = 0;
	for( MyGUI::Widget *root : MyGUI::Gui::getInstance().getRootWidgets() ){
		count += countWidgets( root );
	}
	return count;
}
//-------------------------------------------------------------------------------------
static size_t countResources( Ogre::ResourceManager &manager ){
	size_t count = 0;
	Ogre::ResourceManager::ResourceMapIterator it = manager.getResourceIterator();
	while( it.hasMoreElements() ){
		it.getNext();
		count++;
	}
	return count;
}
//-------------------------------------------------------------------------------------
static size_t countNodes( Ogre::Node *node ){
	size_t count = 1;
	for( Ogre::Node *child : node->getChildren() ){
		count += countNodes( child );
	}
	return count;
}
//-------------------------------------------------------------------------------------
void DevTest::logMemory( const Ogre::String &label ){
	// The Direct3D 11 per-match memory growth (docs/SPECIAL_EFFECTS.md, "Known issue") was measured with these versions.
	// When the graphics driver or Ogre differs, say so: re-check whether it still happens (and whether the workarounds
	// noted there are still needed), then update the doc and these values.
	static bool versionsChecked = false;
	if( !versionsChecked ){
		versionsChecked = true;
		const char* MEASURED_DRIVER = "32.0.16.1714";
		const char* MEASURED_OGRE = "14.6.0";
		Ogre::RenderSystem* renderSystem = Ogre::Root::getSingleton().getRenderSystem();
		Ogre::String driver = renderSystem->getDriverVersion().toString();
		Ogre::String ogre = Ogre::StringConverter::toString( OGRE_VERSION_MAJOR ) + "." + Ogre::StringConverter::toString( OGRE_VERSION_MINOR ) + "." + Ogre::StringConverter::toString( OGRE_VERSION_PATCH );
		bool d3d11 = renderSystem->getName().find( "Direct3D11" ) != Ogre::String::npos;
		if( d3d11 && ( driver != MEASURED_DRIVER || ogre != MEASURED_OGRE ) ){
			log( "REMINDER: graphics driver " + driver + " / Ogre " + ogre + " differ from the ones the Direct3D 11 memory issue was measured with (driver " + MEASURED_DRIVER + ", Ogre " + MEASURED_OGRE + "): re-check it, see docs/SPECIAL_EFFECTS.md \"Known issue\"" );
		}else{
			log( "versions: graphics driver " + driver + ", Ogre " + ogre + ( d3d11 ? " (as measured for the Direct3D 11 memory issue)" : "" ) );
		}
	}
	Ogre::String memory = "?";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	PROCESS_MEMORY_COUNTERS_EX counters;
	if( GetProcessMemoryInfo( GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*) &counters, sizeof( counters ) ) ){
		memory = Ogre::StringConverter::toString( (unsigned long) ( counters.PrivateUsage / 1024 ) ) + "KB";
	}
	// Bytes really in use on the heap malloc/new use (private bytes also count fragmentation and driver memory).
	HANDLE heap = GetProcessHeap();
	PROCESS_HEAP_ENTRY entry = {};
	unsigned long long busyBytes = 0, busyBlocks = 0;
	if( HeapLock( heap ) ){
		while( HeapWalk( heap, &entry ) ){
			if( entry.wFlags & PROCESS_HEAP_ENTRY_BUSY ){
				busyBytes += entry.cbData;
				busyBlocks++;
			}
		}
		HeapUnlock( heap );
		memory += " heap=" + Ogre::StringConverter::toString( (unsigned long) ( busyBytes / 1024 ) ) + "KB/" + Ogre::StringConverter::toString( (unsigned long) busyBlocks );
	}
		// -cycles: which block sizes grow from one match to the next (the biggest growth in bytes), with a peek at the
		// content of a few blocks of the top size, to find what keeps growing.
		static std::map<unsigned long, unsigned long> previous;
		std::map<unsigned long, unsigned long> sizes;
		std::map<unsigned long, std::vector<const unsigned char*> > samples;
		entry = PROCESS_HEAP_ENTRY();
		if( HeapLock( heap ) ){
			while( HeapWalk( heap, &entry ) ){
				if( entry.wFlags & PROCESS_HEAP_ENTRY_BUSY ){
					sizes[entry.cbData]++;
					std::vector<const unsigned char*> &list = samples[entry.cbData];
					if( list.size() < 4 ){
						list.push_back( (const unsigned char*)entry.lpData );
					}else{
						list[list.size() - 1] = (const unsigned char*)entry.lpData;	// keep the last one walked
					}
				}
			}
			HeapUnlock( heap );
		}
		if( !previous.empty() ){
			std::vector< std::pair<long long, unsigned long> > growth;
			for( std::map<unsigned long, unsigned long>::iterator s = sizes.begin(); s != sizes.end(); s++ ){
				long long delta = (long long)s->second - (long long)previous[s->first];
				if( delta > 0 ){
					growth.push_back( std::make_pair( -delta * (long long)s->first, s->first ) );
				}
			}
			std::sort( growth.begin(), growth.end() );
			Ogre::String top;
			for( size_t i = 0; i < growth.size() && i < 8; i++ ){
				top += " " + Ogre::StringConverter::toString( growth[i].second ) + "B+" + Ogre::StringConverter::toString( (unsigned long)( -growth[i].first / growth[i].second ) );
			}
			log( "heap growth by block size:" + top );
			for( size_t g = 0; g < growth.size() && g < 4; g++ ){
				unsigned long size = growth[g].second;
				std::vector<const unsigned char*> &list = samples[size];
				for( size_t i = 0; i < list.size() && i < 2; i++ ){
					Ogre::String text;
					for( unsigned long b = 0; b < size && b < 48; b++ ){
						unsigned char c = list[i][b];
						text += ( c >= 32 && c < 127 ) ? (char)c : '.';
					}
					log( "  sample " + Ogre::StringConverter::toString( size ) + "B: " + text );
					// Pointers inside the block that land in a loaded module (a vtable or code): which DLL owns the object.
					Ogre::String modules;
					for( unsigned long q = 0; q + sizeof(void*) <= size && q < 64; q += sizeof(void*) ){
						const void* pointer = *(const void* const*)( list[i] + q );
						HMODULE module = NULL;
						char name[MAX_PATH];
						if( GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)pointer, &module ) && module != NULL
							&& GetModuleFileNameA( module, name, MAX_PATH ) ){
							Ogre::String path( name );
							modules += " +" + Ogre::StringConverter::toString( q ) + ":" + path.substr( path.find_last_of( "\/" ) + 1 );
						}
					}
					log( "  sample modules:" + modules );
				}
			}
		}
		previous = sizes;
#endif
	{	// Live Direct3D 11 objects by type, when the D3D11 debug layer is on (docs/SPECIAL_EFFECTS.md, "Known issue")
		TUMBU* app = TUMBU::getInstance();
		ID3D11Device* device = NULL;
		if( app->mWindow != NULL && Ogre::Root::getSingleton().getRenderSystem()->getName().find( "Direct3D11" ) != Ogre::String::npos ){
			app->mWindow->getCustomAttribute( "D3DDEVICE", &device );
		}
		ID3D11Debug* debug = NULL;
		ID3D11InfoQueue* queue = NULL;
		if( device != NULL && SUCCEEDED( device->QueryInterface( __uuidof( ID3D11Debug ), (void**)&debug ) )
			&& SUCCEEDED( device->QueryInterface( __uuidof( ID3D11InfoQueue ), (void**)&queue ) ) ){
			queue->ClearStoredMessages();
			queue->SetMessageCountLimit( (UINT64)-1 );
			// Ogre only keeps corruption-level messages: let the report through, then restore its filters.
			queue->PushEmptyStorageFilter();
			queue->PushEmptyRetrievalFilter();
			debug->ReportLiveDeviceObjects( D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL );
			queue->PopStorageFilter();	// back to Ogre's filter before anything else is drawn
			std::map<Ogre::String, int> types;
			UINT64 count = queue->GetNumStoredMessages();
			for( UINT64 m = 0; m < count; m++ ){
				SIZE_T size = 0;
				queue->GetMessage( m, NULL, &size );
				std::vector<char> data( size );
				D3D11_MESSAGE* message = (D3D11_MESSAGE*)&data[0];
				if( SUCCEEDED( queue->GetMessage( m, message, &size ) ) ){
					Ogre::String text( message->pDescription, message->DescriptionByteLength );
					size_t at = text.find( "Live " );
					if( at != Ogre::String::npos ){
						size_t end = text.find_first_of( " ,:", at + 5 );
						types[text.substr( at + 5, end - at - 5 )]++;
					}
				}
			}
			Ogre::String summary;
			for( std::map<Ogre::String, int>::iterator t = types.begin(); t != types.end(); t++ ){
				summary += " " + t->first + "=" + Ogre::StringConverter::toString( t->second );
			}
			queue->PopRetrievalFilter();
			summary += " (messages " + Ogre::StringConverter::toString( (unsigned long)count ) + ")";
			log( "d3d11 live objects:" + summary );
			queue->ClearStoredMessages();
		}
		if( queue != NULL ) queue->Release();
		if( debug != NULL ) debug->Release();
	}
	Ogre::SceneManager *sceneMgr = TUMBU::getInstance()->mSceneMgr;
	Ogre::String objects =
		" nodes=" + Ogre::StringConverter::toString( countNodes( sceneMgr->getRootSceneNode() ) ) +
		" entities=" + Ogre::StringConverter::toString( sceneMgr->getMovableObjects( "Entity" ).size() ) +
		" particles=" + Ogre::StringConverter::toString( sceneMgr->getMovableObjects( "ParticleSystem" ).size() ) +
		" lights=" + Ogre::StringConverter::toString( sceneMgr->getMovableObjects( "Light" ).size() ) +
		" materials=" + Ogre::StringConverter::toString( countResources( Ogre::MaterialManager::getSingleton() ) ) +
		" textures=" + Ogre::StringConverter::toString( countResources( Ogre::TextureManager::getSingleton() ) ) +
		" meshes=" + Ogre::StringConverter::toString( countResources( Ogre::MeshManager::getSingleton() ) ) +
		" skeletons=" + Ogre::StringConverter::toString( countResources( Ogre::SkeletonManager::getSingleton() ) ) +
		" programs=" + Ogre::StringConverter::toString( countResources( Ogre::GpuProgramManager::getSingleton() ) ) +
		" compositors=" + Ogre::StringConverter::toString( countResources( Ogre::CompositorManager::getSingleton() ) ) +
		" widgets=" + Ogre::StringConverter::toString( countWidgets() );
	log( "memory " + label + " private=" + memory + objects );
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	heapHistory.push_back( (unsigned long)( busyBytes / 1024 ) );
#endif
	objectHistory.push_back( objects );
}
//-------------------------------------------------------------------------------------
void DevTest::logMemorySummary(void){
	// -cycles: one line to compare with the expected numbers (CLAUDE.md "Automated checks"), also appended to
	// %USERPROFILE%\Tumbu\memory-history.csv so the heap growth per match is tracked across runs, drivers and Ogre versions.
	// Cycle 0 is the menu before any match; the heap must stay flat from cycle 2 on.
	if( heapHistory.size() < 4 ){
		log( "memory summary: needs -cycles=3 or more" );
		return;
	}
	size_t last = heapHistory.size() - 1;
	double growth = ( (double)heapHistory[last] - (double)heapHistory[2] ) / ( last - 2 );
	bool flat = true;
	for( size_t i = 2; i <= last; i++ ){
		flat = flat && objectHistory[i] == objectHistory[1];
	}
	TUMBU* tumbu = TUMBU::getInstance();
	Ogre::RenderSystem* renderSystem = Ogre::Root::getSingleton().getRenderSystem();
	Ogre::String render = renderSystem->getName().find( "Direct3D11" ) != Ogre::String::npos ? "D3D11" : "GL";
	Ogre::String driver = renderSystem->getDriverVersion().toString();
	Ogre::String ogre = Ogre::StringConverter::toString( OGRE_VERSION_MAJOR ) + "." + Ogre::StringConverter::toString( OGRE_VERSION_MINOR ) + "." + Ogre::StringConverter::toString( OGRE_VERSION_PATCH );
	Ogre::String growthText = Ogre::StringConverter::toString( (Ogre::Real) growth, 5 );
	log( "memory summary: render=" + render + " driver=" + driver + " ogre=" + ogre +
		" matches=" + Ogre::StringConverter::toString( last ) +
		" heap cycle1=" + Ogre::StringConverter::toString( heapHistory[1] ) + "KB cycle2=" + Ogre::StringConverter::toString( heapHistory[2] ) +
		"KB last=" + Ogre::StringConverter::toString( heapHistory[last] ) + "KB growth=" + growthText + "KB/match (cycles 2.." +
		Ogre::StringConverter::toString( last ) + ") objects=" + ( flat ? "flat" : "CHANGED" ) +
		" aa=" + Ogre::StringConverter::toString( tumbu->isAntiAliasingEnabled() ) );

	Ogre::String path = tumbu->workPath + "memory-history.csv";
	bool exists = Ogre::FileSystemLayer::fileExists( path );
	std::ofstream file( path.c_str(), std::ios::app );
	if( file ){
		if( !exists ){
			file << "date,render,driver,ogre,matches,aa,heap_cycle1_kb,heap_cycle2_kb,heap_last_kb,growth_kb_per_match,objects\n";
		}
		char date[32];
		time_t now = time( NULL );
		strftime( date, sizeof( date ), "%Y-%m-%d %H:%M", localtime( &now ) );
		file << date << "," << render << "," << driver << "," << ogre << "," << last << "," << tumbu->isAntiAliasingEnabled() << ","
			<< heapHistory[1] << "," << heapHistory[2] << "," << heapHistory[last] << "," << growthText << "," << ( flat ? "flat" : "CHANGED" ) << "\n";
	}
}
//-------------------------------------------------------------------------------------
void DevTest::runCycles( const Ogre::FrameEvent &evt ){
	TUMBU* tumbu = TUMBU::getInstance();
	TumbuEnums::GameState state = tumbu->getGameState();
	tourTimer += evt.timeSinceLastFrame;

	switch( tourStep ){
	case 0: // start screen
		if( state == TumbuEnums::START_SCREEN && frames > 30 && tourTimer > 0.5f ){
			tourStep++; tourTimer = 0;	// advance first: actions may render frames (re-entering here)
			logMemory( "cycle " + Ogre::StringConverter::toString( cycle ) + " menu" );
			click( "PlayButton" );
		}
		break;
	case 1: // intro dialogs (no tutorial)
	case 3: // dialogs after a kill (won part, next enemy)
		if( state == TumbuEnums::IN_DIALOG ){
			if( tourTimer > 0.05f ){
				tourTimer = 0;
				pressKey( isVisible( "ConfirmWindow" ) ? TumbuInput::KEY_ESCAPE : TumbuInput::KEY_SPACE );
			}
		}else if( state == TumbuEnums::PLAYING && tourTimer > 0.2f ){
			if( tourStep == 3 ) kills++;
			tourStep = 2; tourTimer = 0;
		}
		break;
	case 2: // fight: punch, kick and Jyn (projectiles hit through the collision code), then defeat the enemy
			// (respawn path) or leave the match
		if( state == TumbuEnums::PLAYING ){
			const char attackKeys[] = { 'o', 'u', 'i' };	// punch, kick, Jyn
			if( attacks < 3 && tourTimer > 0.2f + attacks * 0.8f ){
				pressKey( attackKeys[attacks] );
				attacks++;
			}else if( attacks >= 3 && tourTimer > 3.5f ){
				tourTimer = 0;
				attacks = 0;
				if( kills < 3 && tumbu->getDemo() != NULL ){
					tourStep = 3;
					log( "fight: enemy hp " + Ogre::StringConverter::toString( tumbu->getDemo()->enemy->hp ) + " before the kill" );
					tumbu->getDemo()->enemy->hp = 0;
				}else{
					tourStep = 4;
					pressKey( TumbuInput::KEY_ESCAPE );
				}
			}
		}
		break;
	case 4: // pause menu → Quit
		if( tourTimer > 0.3f ){
			tourStep++; tourTimer = 0;
			click( "QuitTab" );
		}
		break;
	case 5: // back on the start screen
		if( state == TumbuEnums::START_SCREEN && tourTimer > 0.5f ){
			cycle++; kills = 0; tourTimer = 0;
			if( cycle < cycles ){
				tourStep = 0;
			}else{
				tourStep = 6;
				logMemory( "cycle " + Ogre::StringConverter::toString( cycle ) + " menu" );
				log( "cycles finished" );
				logMemorySummary();
				click( "ExitButton" );
			}
		}
		break;
	}
}
//-------------------------------------------------------------------------------------
void DevTest::measureWalkCycles(void){
	// Samples the walk and run animations of every legs set in their own model space. A foot on the ground
	// slides backward relative to the body; the body must advance by that much for the foot not to skate.
	Ogre::SceneManager *sm = Ogre::Root::getSingleton().createSceneManager( Ogre::SMT_DEFAULT, "AnimMeasure" );
	const char *anims[] = { "walk", "run" };
	for( int set = 1; set <= 5; set++ ){
		Ogre::String mesh = "legs_00" + Ogre::StringConverter::toString( set ) + ".mesh";
		Ogre::Entity *entity = sm->createEntity( mesh );
		sm->getRootSceneNode()->createChildSceneNode()->attachObject( entity );
		for( const char *anim : anims ){
			Ogre::AnimationState *state = entity->getAnimationState( anim );
			Ogre::AnimationStateSet *all = entity->getAllAnimationStates();
			for( auto &s : all->getAnimationStates() ) s.second->setEnabled( false );
			state->setEnabled( true );
			state->setWeight( 1 );
			const int samples = 400;
			std::vector<Ogre::Vector3> left, right;
			for( int i = 0; i <= samples; i++ ){
				state->setTimePosition( state->getLength() * i / samples );
				// Apply the pose now (the entity itself re-evaluates animation only once per rendered frame).
				entity->getSkeleton()->setAnimationState( *entity->getAllAnimationStates() );
				left.push_back( entity->getSkeleton()->getBone( "ankle_L" )->_getDerivedPosition() );
				right.push_back( entity->getSkeleton()->getBone( "ankle_R" )->_getDerivedPosition() );
			}
			{	// A cycle whose poses mirror in time (t and 1-t alike) swings each leg like a pendulum: the low foot
				// slides back and then forward along the same arc, so no playback rate can match the ground. A
				// real walk cycle plants and pushes back, then lifts and swings forward (asymmetric).
				Ogre::Real mirrorError = 0, lo = left[0].z, hi = left[0].z;
				for( int i = 0; i <= samples; i++ ){
					mirrorError = std::max( mirrorError, left[i].distance( left[samples - i] ) );
					lo = std::min( lo, left[i].z );
					hi = std::max( hi, left[i].z );
				}
				ConfigNode *cfg = ConfigScriptLoader::getSingleton().getConfigScript( "game", "robot" );
				Ogre::Real speed = cfg->findChild( strcmp( anim, "run" ) == 0 ? "runSpeed" : "walkSpeed" )->getValueF( 0 );
				char line[260];
				snprintf( line, sizeof( line ), "cycle %s %s: foot sweep %.3f, %s; matching the pushing half at %.2f u/s needs rate %.2f",
					mesh.c_str(), anim, hi - lo, mirrorError < 0.01f ? "mirror-symmetric pendulum (feet will slide)" : "asymmetric walk cycle",
					speed, speed * state->getLength() / ( 2 * ( hi - lo ) ) );
				log( line );
			}
			for( int side = 0; side < 2; side++ ){
				std::vector<Ogre::Vector3> &p = side == 0 ? left : right;
				Ogre::Vector3 lo = p[0], hi = p[0];
				for( auto &v : p ){ lo.makeFloor( v ); hi.makeCeil( v ); }
				// Ground contact: the lowest 20% of the foot's height range. Sum the backward (-Z) motion there.
				Ogre::Real contactY = lo.y + ( hi.y - lo.y ) * 0.2f;
				Ogre::Real slideZ = 0, slideX = 0;
				int contactSamples = 0;
				for( int i = 1; i <= samples; i++ ){
					if( p[i].y <= contactY && p[i - 1].y <= contactY ){
						slideZ += p[i].z - p[i - 1].z;
						slideX += p[i].x - p[i - 1].x;
						contactSamples++;
					}
				}
				char line[300];
				snprintf( line, sizeof( line ), "anim %s %s ankle_%c: length=%.2fs range x[%.3f %.3f] y[%.3f %.3f] z[%.3f %.3f] ground=%.0f%% slideZ=%.3f slideX=%.3f",
					mesh.c_str(), anim, side == 0 ? 'L' : 'R', state->getLength(), lo.x, hi.x, lo.y, hi.y, lo.z, hi.z,
					100.0f * contactSamples / samples, slideZ, slideX );
				log( line );
			}
		}
		sm->destroyEntity( entity );
	}
	Ogre::Root::getSingleton().destroySceneManager( sm );
}
