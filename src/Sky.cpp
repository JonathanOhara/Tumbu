#include "Sky.h"
#include "TUMBU.h"
#include <Caelum.h>
Sky* Sky::instance = NULL;
//-------------------------------------------------------------------------------------
Sky::Sky(Ogre::SceneManager* sceneMgr){
	mSceneMgr = sceneMgr;

	updateTime = 0;
	caelum = NULL;
	quality = 0;
	clock = NULL;

	timeMultiplier = TUMBU::getInstance()->getTimeMultiplier();

	// The scene's light is the sun; Lighting sets its direction and colour.
	light = mSceneMgr->getLight("skyXSpotLight");
}
//-------------------------------------------------------------------------------------
Sky::~Sky(void){
	if( caelum != NULL ){
		TUMBU::getInstance()->mWindow->removeListener( caelum );
		Ogre::Root::getSingleton().removeFrameListener( caelum );
		caelum->shutdown( true );
		caelum = NULL;
	}
	mSceneMgr->destroyLight( light );
	instance = NULL;
}
//-------------------------------------------------------------------------------------
Sky* Sky::getInstance(){
	if( instance == NULL){
		instance = new Sky(TUMBU::getInstance()->mSceneMgr );
	}
	return instance;
}
//-------------------------------------------------------------------------------------
void Sky::setClock( Clock* _clock ){
	clock = _clock;
	dayType = clock->getDayType();
}
//-------------------------------------------------------------------------------------
void Sky::skyLowQuality(){
	quality = 0;
	skyLowQualityMorning();
}
//-------------------------------------------------------------------------------------
void Sky::skyLowQualityMorning(){
	mSceneMgr->setSkyDome(true, "Tumbu/MorningSky");
}
//-------------------------------------------------------------------------------------
void Sky::skyLowQualityNight(){
	mSceneMgr->setSkyDome(true, "Tumbu/NightSky");
}
//-------------------------------------------------------------------------------------
void Sky::skyHighQuality(Ogre::Camera* camera){
	// Caelum (day/night sky, sun, moon, stars, clouds) replaces the 2011 SkyX sky. Its shaders exist only in
	// HLSL, so other render systems keep the skydome.
	Ogre::String renderSystem = Ogre::Root::getSingleton().getRenderSystem()->getName();
	if( renderSystem.find( "Direct3D11" ) == Ogre::String::npos ){
		Ogre::LogManager::getSingletonPtr()->logMessage( "Sky: the day/night sky needs Direct3D 11 (" + renderSystem + " in use), using the skydome" );
		skyLowQuality();
		return;
	}

	quality = 1;
	mSceneMgr->setSkyDome( false, "" );

	caelum = new Caelum::CaelumSystem( Ogre::Root::getSingletonPtr(), mSceneMgr, Caelum::CaelumSystem::CAELUM_COMPONENTS_DEFAULT );
	caelum->attachViewport( TUMBU::getInstance()->mWindow->getViewport( 0 ) );
	// Before each viewport update Caelum centres the sky on the camera and sizes it to the far clip distance.
	TUMBU::getInstance()->mWindow->addListener( caelum );
	caelum->setTimeScale( 0 );	// the game's Clock drives the time of day (see frameRenderingQueued)
	caelum->setManageAmbientLight( false );
	caelum->setManageSceneFog( Ogre::FOG_NONE );
	// Caelum only draws the sky: the arena keeps its own lights (like the 2011 SkyX sky did). Caelum's sun
	// and moon lights on top of them overexpose the scene at dusk.
	if( caelum->getSun() != NULL ) caelum->getSun()->setForceDisable( true );
	if( caelum->getMoon() != NULL ) caelum->getMoon()->setForceDisable( true );
	Ogre::Root::getSingleton().addFrameListener( caelum );
	updateCaelumTime();
	Ogre::LogManager::getSingletonPtr()->logMessage( "Sky: Caelum day/night sky enabled" );
}
//-------------------------------------------------------------------------------------
void Sky::updateCaelumTime(void){
	if( caelum == NULL || clock == NULL ){
		return;
	}
	float hours = clock->getHours();
	int h = (int) hours;
	int m = (int) ( ( hours - h ) * 60 );
	// 7 August 2011: the last update of the original game.
	caelum->getUniversalClock()->setGregorianDateTime( 2011, 8, 7, h, m, 0 );
}
//-------------------------------------------------------------------------------------
bool Sky::frameRenderingQueued(const Ogre::FrameEvent &evt){
	if( TUMBU::getInstance()->isPlaying() ){
		updateTime += evt.timeSinceLastFrame;

		if( quality == 0){
			if(updateTime >=  10 ){
				updateTime -= 10;
				if( clock != NULL && dayType != clock->getDayType() ){
					dayType = clock->getDayType();

					switch( dayType ){
					case TumbuEnums::MORNING:
						skyLowQualityMorning();
						break;
					case TumbuEnums::NIGHT:
						skyLowQualityNight();
						break;
					}
				}
			}
		}
		else if( quality == 1 ){
			updateCaelumTime();
		}
	}

	return true;
}
