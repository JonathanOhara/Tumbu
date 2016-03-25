#include "Sky.h"
#include "Tumbu.h"
Sky* Sky::instance = NULL;
//-------------------------------------------------------------------------------------
Sky::Sky(Ogre::SceneManager* sceneMgr){
	mSceneMgr = sceneMgr;

	skyXUpdateTime = 0;
	updateTime = 0;

	timeMultiplier = TUMBU::getInstance()->getTimeMultiplier();

	light = mSceneMgr->getLight("skyXSpotLight");
	light->setType(Ogre::Light::LT_POINT);
	light->setDiffuseColour(Ogre::ColourValue::White);
	light->setSpecularColour(Ogre::ColourValue::White);
//	light->setPosition( 10, 20, 0 );
	light->setCastShadows( TUMBU::getInstance()->isCastShadows() );
}
//-------------------------------------------------------------------------------------
Sky::~Sky(void){
	mSceneMgr->destroyLight( light );
	instance = NULL;
#ifdef _WINDOWS
	if(quality == 1){
		delete skyX;
	}
#endif
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
	quality = 1;
	mSceneMgr->setSkyDome(false, "");
#ifdef _WINDOWS	
	skyX = new SkyX::SkyX(mSceneMgr, camera);	
	skyX->create();
	skyX->setTimeMultiplier(timeMultiplier);
	skyX->setLightingMode(SkyX::SkyX::LM_LDR);
	skyX->getCloudsManager()->add(SkyX::CloudLayer::Options(/* Default options */));
#endif
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
#ifdef _WINDOWS
		else if(quality == 1){
			float time = skyX->getAtmosphereManager()->getOptions().Time.x;
			skyX->update( evt.timeSinceLastFrame );
		}
#endif
	}

	return true;
}