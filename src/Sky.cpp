#include "Sky.h"
#include "TUMBU.h"
Sky* Sky::instance = NULL;
//-------------------------------------------------------------------------------------
Sky::Sky(Ogre::SceneManager* sceneMgr){
	mSceneMgr = sceneMgr;
	quality = 1;
	skyNode = NULL;
	skyObject = NULL;

	// The scene's light is the sun; Lighting sets its direction and colour.
	light = mSceneMgr->getLight("skyXSpotLight");
}
//-------------------------------------------------------------------------------------
Sky::~Sky(void){
	if( skyObject != NULL ){
		skyNode->detachAllObjects();
		mSceneMgr->destroyManualObject( skyObject );
		mSceneMgr->destroySceneNode( skyNode );
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
void Sky::skyLowQuality(){
	quality = 0;
	showSky();
}
//-------------------------------------------------------------------------------------
void Sky::skyHighQuality(Ogre::Camera* camera){
	quality = 1;
	showSky();
}
//-------------------------------------------------------------------------------------
void Sky::showSky(void){
	if( skyObject == NULL ){
		// A big box around the arena, drawn first (RENDER_QUEUE_SKIES_EARLY) and writing no depth, so the
		// post-processing still sees sky pixels as "far" (no fog, no god rays on them). The shader works from the
		// direction to the camera, so the box's size does not show. (Ogre's own sky box takes only cube-map
		// materials and swapped this one for its default.)
		const Ogre::Real s = 500;
		skyObject = mSceneMgr->createManualObject( "TumbuToonSky" );
		skyObject->setCastShadows( false );
		skyObject->setRenderQueueGroup( Ogre::RENDER_QUEUE_SKIES_EARLY );
		skyObject->begin( "Tumbu/ToonSky", Ogre::RenderOperation::OT_TRIANGLE_LIST, Ogre::RGN_AUTODETECT );
		for( int i = 0; i < 8; i++ ){
			skyObject->position( ( i & 1 ) ? s : -s, ( i & 2 ) ? s : -s, ( i & 4 ) ? s : -s );
		}
		const int faces[6][4] = { {0,1,3,2}, {4,6,7,5}, {0,4,5,1}, {2,3,7,6}, {0,2,6,4}, {1,5,7,3} };
		for( int f = 0; f < 6; f++ ){
			skyObject->quad( faces[f][0], faces[f][1], faces[f][2], faces[f][3] );
		}
		skyObject->end();
		skyNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
		skyNode->attachObject( skyObject );
	}
	Ogre::LogManager::getSingletonPtr()->logMessage( Ogre::String( "Sky: painted toon sky, " ) + ( quality == 1 ? "High (clouds)" : "Low" ) );
}
//-------------------------------------------------------------------------------------
int Sky::getQuality(void){
	return quality;
}
