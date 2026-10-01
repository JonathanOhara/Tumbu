#include "TUMBU.h"
#include <chrono>
#include <thread>
// Visual Leak Detector (optional): used by RelWithDebInfo builds when its headers are on the include path.
#if defined(TUMBU_DEBUG) && __has_include(<vld.h>)
#include <vld.h>
#endif
TUMBU* TUMBU::instance = NULL;
//-------------------------------------------------------------------------------------
TUMBU::TUMBU(void){
	srand( (unsigned)time( NULL ) );	
	mNumEntitiesInstanced = 0;

	barrelIndex = 0;

	/** OPTIONS */ 
	castShadows = false;
	shadowTechnique = Ogre::SHADOWTYPE_NONE;
	shadowColor = Ogre::ColourValue(0.7,0.7,0.7);
	shadowFarDistance = 50;
	shadowTextureSize = 256;
	shadowTextureCount = 1;
	
	skyQuality = 0;
	frameLimit = -1;	// monitor refresh (VSync)
	timeMultiplier = 0.1f;

	gameState = TumbuEnums::NONE;

	activeScene			= NULL;
	ai					= NULL;
	clock				= NULL;
	cutScene			= NULL;
	demo				= NULL;
	devTest				= NULL;
	gui					= NULL;
	soundManager		= NULL;
	startScreenBackgroundSound = NULL;
}
//-------------------------------------------------------------------------------------
TUMBU::~TUMBU(void){
	instance = NULL;
}
//-------------------------------------------------------------------------------------
void TUMBU::destroyScene(void){
	// Runs before Ogre shuts down (BaseApplication::go), while the Root and the scene still exist.
	if( mRoot == NULL || mSceneMgr == NULL ){
		return;
	}
	Ogre::LogManager::getSingletonPtr()->logMessage("Finishing Game");

	removeAllKeyListeners();
	removeAllMouseListeners();

	// Closing the game during a match: the match goes first, it still uses the sound, AI and GUI managers.
	if( demo != NULL ){
		Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting Demo...");
		mRoot->removeFrameListener( demo );
		delete demo;
		demo = NULL;
	}

	if( devTest != NULL ){
		mRoot->removeFrameListener( devTest );
		delete devTest;
	}

	Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting CutScene...");
	if( cutScene != NULL ){
		mRoot->removeFrameListener( cutScene );
		delete cutScene;
	}
	Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting SoundManager...");
	if( soundManager != NULL ){
		if( startScreenBackgroundSound != NULL ){
			startScreenBackgroundSound->stop();
			soundManager->destroySound( startScreenBackgroundSound );
		}
		mRoot->removeFrameListener( soundManager );
		delete soundManager;
	}
	Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting Clock...");
	if( clock != NULL ){
		mRoot->removeFrameListener( clock );
		delete clock;
	}
	Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting AI...");
	if( ai != NULL ){
		mRoot->removeFrameListener( ai );
		delete ai;
	}
	if ( activeScene != NULL ){
		Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting Active Scene...");
		activeScene->removeAndDestroyAllChildren();
		mSceneMgr->destroySceneNode( activeScene );
	}
	Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting Gui...");
	if ( gui != NULL ){
		mRoot->removeFrameListener( gui );
		removeKeyListener( gui );
		removeMouseListener( gui );
		removeJoystickListener( gui );
		delete gui;
	}

	Ogre::LogManager::getSingletonPtr()->logMessage("\tCleaning Scene...");
	mSceneMgr->destroyAllEntities();
	mSceneMgr->destroyAllAnimations();
	mSceneMgr->destroyAllAnimationStates();
	mSceneMgr->destroyAllLights();
	mSceneMgr->destroyAllManualObjects();
	mSceneMgr->destroyAllMovableObjects();
	mSceneMgr->destroyAllParticleSystems();
	mSceneMgr->destroyAllStaticGeometry();
}
//-------------------------------------------------------------------------------------
TUMBU* TUMBU::getInstance(void){
	if( instance == NULL){
		instance = new TUMBU();
	}
	return instance;
}
//-------------------------------------------------------------------------------------
bool TUMBU::frameRenderingQueued(const Ogre::FrameEvent& evt){
    return BaseApplication::frameRenderingQueued(evt);
}
//-------------------------------------------------------------------------------------
bool TUMBU::frameStarted(const Ogre::FrameEvent& evt){
	// Frame rate limit (options menu). VSync is handled by the window; the dev-test FPS cap takes precedence.
	if( frameLimit > 0 && !DevTest::isEnabled() ){
		const unsigned long frameMicros = 1000000UL / frameLimit;
		unsigned long elapsed = frameLimitTimer.getMicroseconds();
		while( elapsed < frameMicros ){
			unsigned long remainingMillis = ( frameMicros - elapsed ) / 1000;
			if( remainingMillis > 1 ){
				std::this_thread::sleep_for( std::chrono::milliseconds( remainingMillis - 1 ) );
			}
			elapsed = frameLimitTimer.getMicroseconds();
		}
		frameLimitTimer.reset();
	}
	return BaseApplication::frameStarted(evt);
}
//-------------------------------------------------------------------------------------
bool TUMBU::keyPressed( const OgreBites::KeyboardEvent &arg ){	

#ifdef TUMBU_DEBUG
	if( arg.keysym.sym == TumbuInput::KEY_F12 ){
		TUMBU::getInstance()->printSceneHierarchy();
	}
#endif

	return BaseApplication::keyPressed(arg);
}
//-------------------------------------------------------------------------------------
void TUMBU::initializeGUIStuff(void){
	if( gui == NULL ){
		gui = GUI::getInstance();
		mRoot->addFrameListener( gui );
		gui->initOptions();

		addKeyListener( gui , "GuiKeyListener" );
		addMouseListener( gui , "GuiMouseListener" );
		addJoystickListener( gui, "GuiJoyListener" );
	}else{
		gui->reset();
	}
}
//-------------------------------------------------------------------------------------
bool TUMBU::isPlaying(void){
	return gameState == TumbuEnums::PLAYING;
}
//-------------------------------------------------------------------------------------
void TUMBU::initializeUtil(void){
}
//-------------------------------------------------------------------------------------
void TUMBU::renderOneFrame(void){
	mRoot->renderOneFrame();
}
//-------------------------------------------------------------------------------------
Ogre::Root* TUMBU::getRoot(void){
	return mRoot;
}
//-------------------------------------------------------------------------------------
SimpleRigidBody* TUMBU::createSimpleRigidBody(Physics::DynamicsWorld* physicWorld, const Ogre::String &instanceName, const Ogre::String &meshName, const Ogre::Vector3 &pos, const Ogre::Quaternion &q, const Ogre::Real bodyRestitution, const Ogre::Real bodyFriction, bool shadows, TumbuEnums::PhysicObjectTag tag){
	SimpleRigidBody* simpleRigidBody = new SimpleRigidBody( mSceneMgr, tag );

    simpleRigidBody->entity = mSceneMgr->createEntity(instanceName + Ogre::StringConverter::toString(mNumEntitiesInstanced), meshName);
    simpleRigidBody->entity->setCastShadows (shadows);

	trimeshConverter = new Physics::StaticMeshToShapeConverter( simpleRigidBody->entity );
	simpleRigidBody->shape = trimeshConverter->createTrimesh();
    delete trimeshConverter;

    Physics::RigidBody *sceneRigid = new Physics::RigidBody(
        instanceName + "Rigid" + Ogre::StringConverter::toString(mNumEntitiesInstanced),
        physicWorld);

	simpleRigidBody->node = activeScene->createChildSceneNode( instanceName + Ogre::StringConverter::toString(mNumEntitiesInstanced) );
    simpleRigidBody->node->attachObject( simpleRigidBody->entity );
	simpleRigidBody->setOgreBulletRigidBody( sceneRigid );

    sceneRigid->setStaticShape(simpleRigidBody->node, simpleRigidBody->shape, bodyRestitution, bodyFriction, pos);

    mNumEntitiesInstanced++;

    return simpleRigidBody;
}
//-------------------------------------------------------------------------------------
SimpleRigidBody* TUMBU::createSimpleRigidBody(Physics::DynamicsWorld* physicWorld, Ogre::Entity* entity, Ogre::SceneNode* node, const Ogre::Vector3 &pos, const Ogre::Quaternion &q, const Ogre::Real bodyRestitution, const Ogre::Real bodyFriction, bool shadows, TumbuEnums::PhysicObjectTag tag){
	SimpleRigidBody* simpleRigidBody = new SimpleRigidBody( mSceneMgr, tag );

    simpleRigidBody->entity = entity;
    simpleRigidBody->entity->setCastShadows( shadows );

	trimeshConverter = new Physics::StaticMeshToShapeConverter( simpleRigidBody->entity );
	simpleRigidBody->shape = trimeshConverter->createTrimesh();
    delete trimeshConverter;

    Physics::RigidBody *sceneRigid = new Physics::RigidBody(
		entity->getName() + "Rigid" + Ogre::StringConverter::toString( mNumEntitiesInstanced ),
        physicWorld );

	simpleRigidBody->node =node;
	simpleRigidBody->setOgreBulletRigidBody( sceneRigid );

    sceneRigid->setStaticShape( simpleRigidBody->node, simpleRigidBody->shape, bodyRestitution, bodyFriction, pos );

    mNumEntitiesInstanced++;

    return simpleRigidBody;
}
//-------------------------------------------------------------------------------------
void TUMBU::shutdown(void){
	mShutDown = true;
}
//-------------------------------------------------------------------------------------
void TUMBU::initializeDemo(void){
	startScreenBackgroundSound->stop();
	soundManager->destroySound( startScreenBackgroundSound );
	startScreenBackgroundSound = NULL;

	demo = new Demo();
	demo->initializeDemo();
	mRoot->addFrameListener( demo );

	mRoot->removeFrameListener( cutScene );
	renderOneFrame();	
	delete cutScene;

	cutScene = NULL;
	renderOneFrame();

	// A few frames of the finished match behind the loading cover load the remaining textures and fill the
	// shadow map, so nothing pops in when the cover comes down.
	gui->startLoad( "Starting..." );
	for( int i = 0; i < 5; i++ ){
		renderOneFrame();
	}
	gui->stopLoad();
}
//-------------------------------------------------------------------------------------
void TUMBU::finishDemo(void){
	gameState = TumbuEnums::NONE;

	mRoot->removeFrameListener( demo );
	delete demo;
	demo = NULL;
	activeScene = NULL;

	mSceneMgr->destroyAllEntities();
	mSceneMgr->destroyAllLights();
	mSceneMgr->clearScene();

	cutScene = new StartScreen(mSceneMgr);
	mRoot->addFrameListener( cutScene );

	createStartMenu();
}
//------------------------------------------------------------------------------------
void TUMBU::createScene(void){
	loadOptions();

	cutScene = new StartScreen(mSceneMgr);
	mRoot->addFrameListener( cutScene );
	renderOneFrame();

	ai = AIManager::getInstance();
	mRoot->addFrameListener( ai );
	renderOneFrame();

	soundManager = SoundManager::getInstance();
	mRoot->addFrameListener( soundManager );
	renderOneFrame();	

	clock = Clock::getInstance();
	mRoot->addFrameListener( clock );
	renderOneFrame();

	initializeUtil();
	renderOneFrame();

	createStartMenu();

	if( DevTest::isEnabled() ){
		devTest = new DevTest();
		mRoot->addFrameListener( devTest );
	}
}
//-------------------------------------------------------------------------------------
void TUMBU::createStartMenu(void){
	initializeGUIStuff();
	renderOneFrame();

	cutScene->showStartScreenImage();
	gui->showStartMenu();
	renderOneFrame();

	gameState = TumbuEnums::START_SCREEN;

	startScreenBackgroundSound = soundManager->createSound( "startScreenBackgroundSound", "intro_music.ogg", Ogre::Vector3(0,0,0), true, false, false );
	startScreenBackgroundSound->play();
}
//-------------------------------------------------------------------------------------
//------------------------ GETTERS AND SETTERS ----------------------------------------
//-------------------------------------------------------------------------------------
void TUMBU::setGameState( TumbuEnums::GameState _gameState ){
	gameState = _gameState;
}
//-------------------------------------------------------------------------------------
TumbuEnums::GameState TUMBU::getGameState(){
	return gameState;
}
//-------------------------------------------------------------------------------------
void TUMBU::setActiveSceneNode( Ogre::SceneNode* _activeScene ){
	activeScene = _activeScene;
}
//-------------------------------------------------------------------------------------
Ogre::SceneNode* TUMBU::getActiveSceneNode(){
	return activeScene;
}
//-------------------------------------------------------------------------------------
void TUMBU::setAIManager( AIManager* _ai ){
	ai = _ai;
}
//-------------------------------------------------------------------------------------
AIManager* TUMBU::getAIManager(){
	return ai;
}
//-------------------------------------------------------------------------------------
void TUMBU::setCastShadows( bool _castShadows ){
	castShadows = _castShadows;
}
//-------------------------------------------------------------------------------------
bool TUMBU::isCastShadows(){
	return castShadows;
}
//-------------------------------------------------------------------------------------
void TUMBU::setShadowFarDistance( int _shadowFarDistance ){
	shadowFarDistance = _shadowFarDistance;
}
//-------------------------------------------------------------------------------------
int TUMBU::getShadowFarDistance(){
	return shadowFarDistance;
}
//-------------------------------------------------------------------------------------
void TUMBU::setShadowTextureSize( int _shadowTextureSize ){
	shadowTextureSize = _shadowTextureSize;
}
//-------------------------------------------------------------------------------------
int TUMBU::getShadowTextureSize(){
	return shadowTextureSize;
}
//-------------------------------------------------------------------------------------
void TUMBU::setShadowTextureCount( int _shadowTextureCount ){
	shadowTextureCount = _shadowTextureCount;
}
//-------------------------------------------------------------------------------------
int TUMBU::getShadowTextureCount(){
	return shadowTextureCount;
}
//-------------------------------------------------------------------------------------
void TUMBU::setSkyQuality( int _skyQuality ){
	skyQuality = _skyQuality;
}
//-------------------------------------------------------------------------------------
int TUMBU::getSkyQuality(){
	return skyQuality;
}
//-------------------------------------------------------------------------------------
void TUMBU::setShadowTechnique( Ogre::ShadowTechnique _shadowTechnique ){
	shadowTechnique = _shadowTechnique;
}
//-------------------------------------------------------------------------------------
Ogre::ShadowTechnique TUMBU::getShadowTechnique(){
	return shadowTechnique;
}
//-------------------------------------------------------------------------------------
void TUMBU::setShadowColor( Ogre::ColourValue _shadowColor ){
	shadowColor = _shadowColor;
}
//-------------------------------------------------------------------------------------
Ogre::ColourValue TUMBU::getShadowColor(){
	return shadowColor;
}
//-------------------------------------------------------------------------------------
void TUMBU::setTimeMultiplier( Ogre::Real _timeMultiplier ){
	timeMultiplier = _timeMultiplier;
}
//-------------------------------------------------------------------------------------
void TUMBU::setClock( Clock* _clock ){
	clock = _clock;
}
//-------------------------------------------------------------------------------------
Clock* TUMBU::getClock( ){
	return clock;
}
//-------------------------------------------------------------------------------------
Ogre::Real TUMBU::getTimeMultiplier(){
	return timeMultiplier;
}
//-------------------------------------------------------------------------------------
Demo* TUMBU::getDemo(){
	return demo;
}
//-------------------------------------------------------------------------------------
void TUMBU::printSceneHierarchy(void){
	// Debug key F12 (RelWithDebInfo): the scene graph goes to ogre.log.
	Ogre::String tree = "Scene node hierarchy:\nRoot\n";
	int children = mSceneMgr->getRootSceneNode()->numChildren();
	for( int i = 0; i < children; i++ ){
		printSceneChildren( static_cast <Ogre::SceneNode*> ( mSceneMgr->getRootSceneNode()->getChild( i ) ), 1, tree );
	}
	Ogre::LogManager::getSingleton().logMessage( tree );
}
//-------------------------------------------------------------------------------------
void TUMBU::printSceneChildren( Ogre::SceneNode* node, int level, Ogre::String &tree ){
	tree += Ogre::String( level, '-' ) + ">" + node->getName() + "\n";

	int children = node->numChildren();
	for( int i = 0; i < children; i++ ){
		printSceneChildren( static_cast <Ogre::SceneNode*> ( node->getChild( i ) ), level + 1, tree );
	}
}
//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------
void TUMBU::setShadowPreset( int preset ){
	// 0 = off, 1 = normal, 2 = high. The robot and arena shaders sample the depth shadow map themselves
	// (integrated texture shadows, see TumbuToon.h), so a shadow takes the same toon colour as the unlit side.
	switch( preset ){
	case 1:
		setCastShadows( true );
		setShadowTechnique( Ogre::SHADOWTYPE_TEXTURE_MODULATIVE_INTEGRATED );
		// The arena is about 20 units across: a short shadow distance keeps the shadow map sharp.
		setShadowFarDistance( 40 );
		setShadowTextureSize( 2048 );
		break;
	case 2:
		setCastShadows( true );
		setShadowTechnique( Ogre::SHADOWTYPE_TEXTURE_MODULATIVE_INTEGRATED );
		setShadowFarDistance( 40 );
		setShadowTextureSize( 4096 );
		break;
	default:
		setCastShadows( false );
		setShadowTechnique( Ogre::SHADOWTYPE_NONE );
		setShadowFarDistance( 100 );
		setShadowTextureSize( 256 );
		break;
	}
	setShadowColor( Ogre::ColourValue( 0.4f, 0.4f, 0.4f ) );
	setShadowTextureCount( 1 );
}
//-------------------------------------------------------------------------------------
int TUMBU::getShadowPreset(void){
	if( shadowTechnique == Ogre::SHADOWTYPE_NONE ){
		return 0;
	}
	return shadowTextureSize >= 4096 ? 2 : 1;
}
//-------------------------------------------------------------------------------------
void TUMBU::setFrameLimit( int fps ){
	frameLimit = fps;
	if( mWindow != NULL ){
		mWindow->setVSyncEnabled( frameLimit == -1 );
	}
	frameLimitTimer.reset();
}
//-------------------------------------------------------------------------------------
int TUMBU::getFrameLimit(void){
	return frameLimit;
}
//-------------------------------------------------------------------------------------
void TUMBU::saveOptions(void){
	std::ofstream file( ( workPath + "options.cfg" ).c_str() );
	if( file ){
		file << "# TUMBU options (written by the options menu)\n";
		file << "SkyQuality=" << skyQuality << "\n";
		file << "Shadows=" << getShadowPreset() << "\n";
		file << "FrameLimit=" << frameLimit << "\n";
	}
}
//-------------------------------------------------------------------------------------
void TUMBU::loadOptions(void){
	Ogre::String path = workPath + "options.cfg";
	if( Ogre::FileSystemLayer::fileExists( path ) ){
		Ogre::ConfigFile cfg;
		cfg.load( path );
		setSkyQuality( Ogre::StringConverter::parseInt( cfg.getSetting( "SkyQuality", Ogre::BLANKSTRING, "0" ) ) );
		setShadowPreset( Ogre::StringConverter::parseInt( cfg.getSetting( "Shadows", Ogre::BLANKSTRING, "0" ) ) );
		setFrameLimit( Ogre::StringConverter::parseInt( cfg.getSetting( "FrameLimit", Ogre::BLANKSTRING, "-1" ) ) );
	}else{
		setShadowPreset( 1 );	// no options saved yet: shadows on
		setFrameLimit( frameLimit );
	}
}
//-------------------------------------------------------------------------------------
