#include "TUMBU.h"
#ifdef _DEBUG
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
	timeMultiplier = 0.1f;

	gameState = TumbuEnums::NONE;

	activeScene			= NULL;
	ai					= NULL;
	clock				= NULL;
	cutScene			= NULL;
	demo				= NULL;
	gui					= NULL;
	soundManager		= NULL;
	startScreenBackgroundSound = NULL;
}
//-------------------------------------------------------------------------------------
TUMBU::~TUMBU(void){
	Ogre::LogManager::getSingletonPtr()->logMessage("Finishing Game");
	instance = NULL;

	removeAllKeyListeners();
	removeAllMouseListeners();

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
	return BaseApplication::frameStarted(evt);
}
//-------------------------------------------------------------------------------------
bool TUMBU::keyPressed( const OIS::KeyEvent &arg ){	

#ifdef _DEBUG
	if( arg.key == OIS::KC_F12 ){
		TUMBU::getInstance()->printSceneHierarchy();
	}
#endif

	return BaseApplication::keyPressed(arg);
}
//-------------------------------------------------------------------------------------
bool TUMBU::keyReleased( const OIS::KeyEvent &arg ){
	return BaseApplication::keyReleased( arg );
}
//-------------------------------------------------------------------------------------
bool TUMBU::mouseMoved( const OIS::MouseEvent &arg ){
	return BaseApplication::mouseMoved(arg);
}
//-------------------------------------------------------------------------------------
bool TUMBU::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id ){
	return BaseApplication::mousePressed( arg, id );
}
//-------------------------------------------------------------------------------------
bool TUMBU::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id ){
	return BaseApplication::mouseReleased( arg, id );
}
//-------------------------------------------------------------------------------------
bool TUMBU::povMoved( const OIS::JoyStickEvent &e, int pov ) { 
	return BaseApplication::povMoved( e, pov );
}
//-------------------------------------------------------------------------------------
bool TUMBU::axisMoved( const OIS::JoyStickEvent &e, int axis ) {
	return BaseApplication::axisMoved( e, axis );
}
//-------------------------------------------------------------------------------------
bool TUMBU::sliderMoved( const OIS::JoyStickEvent &e, int sliderID ) {
	return BaseApplication::sliderMoved( e, sliderID );
}
//------------------------------------------------------------------------------------- 
bool TUMBU::buttonPressed( const OIS::JoyStickEvent &e, int button ) {
	return BaseApplication::buttonPressed( e, button );
}
//-------------------------------------------------------------------------------------
bool TUMBU::buttonReleased( const OIS::JoyStickEvent &e, int button ) {
	return BaseApplication::buttonReleased( e, button );
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
SimpleRigidBody* TUMBU::createSimpleRigidBody(OgreBulletDynamics::DynamicsWorld* physicWorld, const Ogre::String &instanceName, const Ogre::String &meshName, const Ogre::Vector3 &pos, const Ogre::Quaternion &q, const Ogre::Real bodyRestitution, const Ogre::Real bodyFriction, bool shadows, TumbuEnums::PhysicObjectTag tag){
	SimpleRigidBody* simpleRigidBody = new SimpleRigidBody( mSceneMgr, tag );

    simpleRigidBody->entity = mSceneMgr->createEntity(instanceName + Ogre::StringConverter::toString(mNumEntitiesInstanced), meshName);
    simpleRigidBody->entity->setCastShadows (shadows);

	trimeshConverter = new OgreBulletCollisions::StaticMeshToShapeConverter( simpleRigidBody->entity );
	simpleRigidBody->shape = trimeshConverter->createTrimesh();
    delete trimeshConverter;

    OgreBulletDynamics::RigidBody *sceneRigid = new OgreBulletDynamics::RigidBody(
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
SimpleRigidBody* TUMBU::createSimpleRigidBody(OgreBulletDynamics::DynamicsWorld* physicWorld, Ogre::Entity* entity, Ogre::SceneNode* node, const Ogre::Vector3 &pos, const Ogre::Quaternion &q, const Ogre::Real bodyRestitution, const Ogre::Real bodyFriction, bool shadows, TumbuEnums::PhysicObjectTag tag){
	SimpleRigidBody* simpleRigidBody = new SimpleRigidBody( mSceneMgr, tag );

    simpleRigidBody->entity = entity;
    simpleRigidBody->entity->setCastShadows( shadows );

	trimeshConverter = new OgreBulletCollisions::StaticMeshToShapeConverter( simpleRigidBody->entity );
	simpleRigidBody->shape = trimeshConverter->createTrimesh();
    delete trimeshConverter;

    OgreBulletDynamics::RigidBody *sceneRigid = new OgreBulletDynamics::RigidBody(
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
}
//-------------------------------------------------------------------------------------
void TUMBU::finishDemo(void){
	gameState = TumbuEnums::NONE;

	mRoot->removeFrameListener( demo );
	delete demo;
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
}
//-------------------------------------------------------------------------------------
void TUMBU::createStartMenu(void){
	initializeGUIStuff();
	renderOneFrame();

	cutScene->showStartScreenImage();
	gui->showStartMenu();
	renderOneFrame();

	gameState = TumbuEnums::START_SCREEN;

	startScreenBackgroundSound = soundManager->createSound( "startScreenBackgroundSound", "intro_music.ogg", Ogre::Vector3(0,0,0), true, false);
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
	cout << "Scene Node Hierarchy" << endl;

	int children = mSceneMgr->getRootSceneNode()->numChildren();
	cout << "Root " << endl;
	for( int i = 0; i < children; i++ ){
		printSceneChildren( static_cast <Ogre::SceneNode*> ( mSceneMgr->getRootSceneNode()->getChild( i ) ), 1 );
	}
}
//-------------------------------------------------------------------------------------
void TUMBU::printSceneChildren( Ogre::SceneNode* node, int level ){

	
	for( int i = 0; i < level ; i++){
		cout << "-";
	}

	cout << ">";
	cout << node->getName() << endl;

	int children = node->numChildren();
	for( int i = 0; i < children; i++ ){
		printSceneChildren( static_cast <Ogre::SceneNode*> ( node->getChild( i ) ), level + 1 );
	}
}
//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------