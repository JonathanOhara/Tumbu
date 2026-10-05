#include "Demo.h"
#include "DevTest.h"
//-------------------------------------------------------------------------------------
Demo::Demo(){
	gui				= GUI::getInstance();
	soundManager	= SoundManager::getInstance();
	tumbu			= TUMBU::getInstance();
	mSceneMgr		= tumbu->mSceneMgr;

	arenaRigidBody				= NULL;
	backGroundSound				= NULL;
	colliseumRigidBody			= NULL;
#ifdef TUMBU_DEBUG
	debugDrawerNode				= NULL;
#endif
	enemy						= NULL;
	mainChar					= NULL;
	tutorial					= NULL;

	terrainRigidBody			= NULL;
	camera						= NULL;
	mLoader						= NULL;
	pDataConvert				= NULL;
	physicWorld					= NULL;
	sky							= NULL;
	lighting					= NULL;
	flyCamera					= NULL;
	dust						= NULL;
	effects						= NULL;
	dustNode					= NULL;
}
//-------------------------------------------------------------------------------------
Demo::~Demo(void){
	Ogre::LogManager::getSingletonPtr()->logMessage("Finishing Demo");

	Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting Tutorial...");
	if( tutorial != NULL ){
		tumbu->getRoot()->removeFrameListener( tutorial );
		tumbu->removeKeyListener( tutorial );
		tumbu->removeMouseListener( tutorial );
		tumbu->removeJoystickListener( tutorial );
		delete tutorial;
	}
	Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting MainChar...");
	if( mainChar != NULL ){
		tumbu->getRoot()->removeFrameListener( mainChar );
		tumbu->removeKeyListener( mainChar );
		tumbu->removeMouseListener( mainChar );
		tumbu->removeJoystickListener( mainChar );
		delete mainChar;
	}
	Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting Enemy...");
	if( enemy != NULL ){
		tumbu->getRoot()->removeFrameListener( enemy );
		delete enemy;
	}
	if( dust != NULL ){
		mSceneMgr->destroyParticleSystem( dust );
		mSceneMgr->destroySceneNode( dustNode );
	}
	Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting Effects...");
	if( effects != NULL ){
		tumbu->getRoot()->removeFrameListener( effects );
		delete effects;
	}
	Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting Lighting...");
	if( lighting != NULL ){
		tumbu->getRoot()->removeFrameListener( lighting );
		delete lighting;
	}
	Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting Sky...");
	if( sky != NULL ){
		tumbu->getRoot()->removeFrameListener( sky );
		delete sky;
	}
	if( flyCamera != NULL ){
		tumbu->getRoot()->removeFrameListener( flyCamera );
		tumbu->removeKeyListener( flyCamera );
		tumbu->removeMouseListener( flyCamera );
		delete flyCamera;
	}
	Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting Camera...");
	if( camera != NULL ){
		tumbu->getRoot()->removeFrameListener( camera );
		tumbu->removeKeyListener( camera );
		tumbu->removeMouseListener( camera );
		tumbu->removeJoystickListener( camera );
		delete camera;
	}
	Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting BackGround Sound...");
	if( backGroundSound != NULL ){
		backGroundSound->stop();
		soundManager->destroySound( backGroundSound );
	}
#ifdef TUMBU_DEBUG
	if( debugDrawerNode != NULL ){
		getPhysicWorld()->setDebugDrawNode( NULL );
		mSceneMgr->destroySceneNode( debugDrawerNode );
	}
#endif
	Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting Arena...");
	if ( arenaRigidBody != NULL ){
		delete arenaRigidBody;
	}
	Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting Colliseum...");
	if ( colliseumRigidBody != NULL ){
		delete colliseumRigidBody;
	}
	Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting Terrain...");
	if ( terrainRigidBody != NULL ){
		delete terrainRigidBody;
		delete[] pDataConvert;
	}
	Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting Phycs...");
	if ( physicWorld != NULL ){
		delete physicWorld;
	}
	
	Ogre::LogManager::getSingletonPtr()->logMessage("\tReseting AI...");
	tumbu->getAIManager()->reset();
	Robot::instances = 0;

	Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting Dot Scene Loader...");
	delete mLoader;

	Ogre::LogManager::getSingletonPtr()->logMessage("\tDemo Finished");
}
//-------------------------------------------------------------------------------------
bool Demo::frameRenderingQueued(const Ogre::FrameEvent &evt){
	const Ogre::Real timeSinceLastFrame = EffectsManager::gameTime( evt.timeSinceLastFrame );	// slowed down during a hit-stop

	switch( tumbu->getGameState() ){
	case TumbuEnums::PLAYING:
		// Fixed 120 Hz physics with up to 8 sub-steps per frame, so the simulation runs at the same speed at any frame rate.
		getPhysicWorld()->stepSimulation(timeSinceLastFrame, 8, 1.0f / 120.0f);
		collisionDetection();

		verifyDeaths();
		break;
	case TumbuEnums::START_SCREEN:
		break;
	case TumbuEnums::PAUSED:
		break;
	}

	return true;
}
//-------------------------------------------------------------------------------------
void Demo::collisionDetection(void){
	btPersistentManifold* contactManifold;
	const btCollisionObject *rigid,
							*rigid2;

	CollisionDetectionListener *listener1,
							   *listener2;

	std::string rigidName,
				rigid2Name;

	getPhysicWorld()->getBulletCollisionWorld()->performDiscreteCollisionDetection();

	const unsigned int numManifolds = getPhysicWorld()->getBulletCollisionWorld()->getDispatcher()->getNumManifolds();
	for (unsigned int i = 0; i < numManifolds; i++){
		contactManifold =  getPhysicWorld()->getBulletCollisionWorld()->getDispatcher()->getManifoldByIndexInternal(i);

		rigid = contactManifold->getBody0();
		rigid2 = contactManifold->getBody1();

		if( getPhysicWorld()->findObject( rigid ) == NULL || getPhysicWorld()->findObject( rigid2 ) == NULL ){
			continue;
		}

		rigidName  = getPhysicWorld()->findObject( rigid )->getName();
		rigid2Name = getPhysicWorld()->findObject( rigid2 )->getName();
		listener1 = TUMBU::getInstance()->getCollisionListenerByName( rigidName );
		listener2 = TUMBU::getInstance()->getCollisionListenerByName( rigid2Name );

		if( listener1 != NULL && listener2 != NULL ){

			int numContacts = contactManifold->getNumContacts();
		
			if( numContacts > 0 ){
				listener1->collisionPosition = Physics::BtOgreConverter::to( contactManifold->getContactPoint(0).getPositionWorldOnA() );
				listener2->collisionPosition = Physics::BtOgreConverter::to( contactManifold->getContactPoint(0).getPositionWorldOnB() );

				listener2->rigidBodyName = rigid2Name;
				listener1->collision( listener2 );
				listener2->rigidBodyName = "";

				listener1->rigidBodyName = rigidName;
				listener2->collision( listener1 );
				listener1->rigidBodyName = "";

				listener1->collisionPosition = Ogre::Vector3::ZERO;
				listener2->collisionPosition = Ogre::Vector3::ZERO;
			}
		}
	}
}
//-------------------------------------------------------------------------------------
void Demo::initializeDemo(){

	Ogre::LogManager::getSingleton().logMessage( "[Demo] Loading scripts" );
	gui->startLoad("Loading Scripts...");
	initialiseGameResources();

	Ogre::LogManager::getSingleton().logMessage( "[Demo] Loading scene" );
	gui->startLoad("Loading Scene...");
	tumbu->renderOneFrame();
	mLoader = new DotSceneLoader();
	tumbu->renderOneFrame();
	// Before the scene: the terrain bakes its light map from the sun direction.
	lighting = Lighting::getInstance();
	lighting->setClock( tumbu->getClock() );
	mLoader->parseDotScene("Arena.scene", "General", mSceneMgr);
	waitForTerrainLighting();

	gui->startLoad("Loading Scene...");
	tumbu->renderOneFrame();
	demoScene = mSceneMgr->getSceneNode("demoScene");
	tumbu->renderOneFrame();
	tumbu->setActiveSceneNode( demoScene );
	tumbu->renderOneFrame();

	Ogre::LogManager::getSingleton().logMessage( "[Demo] Creating lights" );
	gui->startLoad("Loading Lights...");
	tumbu->renderOneFrame();
	createLightEffects();
 
	Ogre::LogManager::getSingleton().logMessage( "[Demo] Creating physics" );
	gui->startLoad("Loading Physics...");
	tumbu->renderOneFrame();
	initializePhysicsStuff();

	Ogre::LogManager::getSingleton().logMessage( "[Demo] Creating sky" );
	gui->startLoad("Loading Sky...");
	tumbu->renderOneFrame();
	createSky();


	Ogre::LogManager::getSingleton().logMessage( "[Demo] Creating terrain physics" );
	gui->startLoad("Loading Terrain...");
	tumbu->renderOneFrame();
	createTerrainPhysic();

	Ogre::LogManager::getSingleton().logMessage( "[Demo] Creating arena physics" );
	gui->startLoad("Loading Terrain...");
	tumbu->renderOneFrame();
	createArena();
	createDust();

	Ogre::LogManager::getSingleton().logMessage( "[Demo] Creating hero" );
	gui->startLoad("Loading Character...");
	tumbu->renderOneFrame();
	createMainCharacter();

	Ogre::LogManager::getSingleton().logMessage( "[Demo] Creating enemy" );
	gui->startLoad("Loading Character...");
	tumbu->renderOneFrame();
	createEnemyCharacter();

	Ogre::LogManager::getSingleton().logMessage( "[Demo] Creating camera" );
	gui->startLoad("Loading Camera...");
	tumbu->renderOneFrame();
	setupCamera();
	
	backGroundSound = soundManager->createSound( "backgroundSound", "battle_music.ogg", Ogre::Vector3(0,0,0), true, false, false );
	backGroundSound->play();

	// The loading cover comes down in TUMBU::initializeDemo, once the match has rendered behind it.

	tumbu->renderOneFrame();

	startInitialConversation();
	gui->hideStartMenu();	
	gui->showBattleLayout();

	mainChar->addEnemy( enemy );
	enemy->addEnemy( mainChar );

	ConfigNode *cfg;
	cfg = ConfigScriptLoader::getSingleton().getConfigScript("demo", "configuration");
	
	TOTAL_ENEMIES = cfg->findChild("enemies")->getValueI();
	numberOfenemies = 0;

	tumbu->setGameState( TumbuEnums::IN_DIALOG );
}
//-------------------------------------------------------------------------------------
void Demo::startTutorialMode(void){
	tutorial = new Tutorial( mainChar, enemy );
	
	tumbu->getRoot()->addFrameListener( tutorial );
	tumbu->addKeyListener( tutorial, "TutorialKeyListener" );
	tumbu->addMouseListener( tutorial, "TutorialMouseListener" );
	tumbu->addJoystickListener( tutorial, "TutorialJoyListener" );

	tutorial->startTutorial();
}
//-------------------------------------------------------------------------------------
void Demo::stopTutorialMode(void){
	if( tutorial != NULL ) {
		tumbu->getRoot()->removeFrameListener( tutorial );
		tumbu->removeKeyListener( tutorial );
		tumbu->removeJoystickListener( tutorial);
		tumbu->removeMouseListener( tutorial );

		tumbu->getRoot()->renderOneFrame();
		delete tutorial;
		tutorial = NULL;
	}
}
//-------------------------------------------------------------------------------------
void Demo::initializePhysicsStuff(void){
	physicWorld = new Physics::DynamicsWorld( mSceneMgr, 
		Ogre::AxisAlignedBox( Ogre::Vector3 (-100, -100, -100), Ogre::Vector3 (100,  100,  100) ), 
		Ogre::Vector3(0,-9.81f,0)
	);

	#ifdef TUMBU_DEBUG
	if( debugDrawerNode == NULL ){
		// Collision shapes drawn as lines (RelWithDebInfo builds).
		debugDrawerNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("debugDrawer", Ogre::Vector3::ZERO);
		getPhysicWorld()->setDebugDrawNode( debugDrawerNode );
	}
	#endif
}
//-------------------------------------------------------------------------------------
void Demo::createDust(void){
	// Dust motes floating over the arena; they shine only in sunlight, so they sparkle in the god rays. Below the
	// wall tops: above them all the air is sunlit and the motes would look like stars in the sky.
	dust = mSceneMgr->createParticleSystem( "ArenaDust", "Tumbu/ArenaDust" );
	dust->setCastShadows( false );
	dustNode = mSceneMgr->getRootSceneNode()->createChildSceneNode( "ArenaDustNode", Ogre::Vector3( 0, 5, 0 ) );
	dustNode->attachObject( dust );
	dust->fastForward( 12 );	// already floating when the match starts
}
//-------------------------------------------------------------------------------------
void Demo::createArena(void){
	Ogre::SceneNode 
		*arenaNode		= mSceneMgr->getSceneNode("arenaSceneNode"),
		*coliseumNode	= mSceneMgr->getSceneNode("coliseumSceneNode");

	Ogre::Entity
		*arenaEntity	= mSceneMgr->getEntity( "arenaEntity" ),
		*coliseumEntity = mSceneMgr->getEntity( "coliseumEntity" );

	arenaRigidBody		= tumbu->createSimpleRigidBody( physicWorld, arenaEntity, arenaNode, Ogre::Vector3(0,0,0), Ogre::Quaternion::IDENTITY, 0.1f, 0.8f, tumbu->isCastShadows(), TumbuEnums::SCENE_OBJECT );
	colliseumRigidBody	= tumbu->createSimpleRigidBody( physicWorld, coliseumEntity, coliseumNode, Ogre::Vector3(0,0,0), Ogre::Quaternion::IDENTITY, 0.1f, 0.8f, tumbu->isCastShadows(), TumbuEnums::SCENE_OBJECT );
}
//-------------------------------------------------------------------------------------
void Demo::createTerrainPhysic(void){
	Ogre::Terrain* pTerrain;
	Ogre::SceneNode* pTerrainNode;
	Physics::CollisionShape* mTerrainShape;
	Physics::RigidBody* defaultTerrainBody;

	pTerrain = mLoader->getTerrainGroup()->getTerrain( 0, 0 );

    float* terrainHeightData = pTerrain->getHeightData();
    Ogre::Vector3 terrainPosition = pTerrain->getPosition();

    pDataConvert= new float[pTerrain->getSize() *pTerrain->getSize()];
    for(int i=0;i<pTerrain->getSize();i++)
		memcpy(
		pDataConvert+pTerrain->getSize() * i, // source
			terrainHeightData + pTerrain->getSize() * (pTerrain->getSize()-i-1), // target
			sizeof(float)*(pTerrain->getSize()) // size
		);

    float metersBetweenVertices = pTerrain->getWorldSize()/(pTerrain->getSize()-1); //edit: fixed 0 -> 1 on 2010-08-13
	Ogre::Vector3 localScaling(metersBetweenVertices, 1, metersBetweenVertices);

	const float      terrainBodyRestitution  = 0.1f;
	const float      terrainBodyFriction     = 0.8f;

	mTerrainShape = new Physics::HeightmapCollisionShape(
		pTerrain->getSize(), 
		pTerrain->getSize(),	
		localScaling,
		pDataConvert,
		pTerrain->getMaxHeight(), 
		pTerrain->getMinHeight(), 
		true);

	defaultTerrainBody = new Physics::RigidBody(
		"Terrain", 
		getPhysicWorld());

	Ogre::Vector3 position(
		terrainPosition.x,
		terrainPosition.y + (pTerrain->getMaxHeight()-1.5f)/2,
		terrainPosition.z
	);

	pTerrainNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("terrain_node");
	defaultTerrainBody->setStaticShape (pTerrainNode, mTerrainShape, terrainBodyRestitution, terrainBodyFriction, position);

	defaultTerrainBody->getBulletRigidBody()->getWorldTransform().setRotation(
		btQuaternion(
			Ogre::Quaternion::IDENTITY.x,
			Ogre::Quaternion::IDENTITY.y,
			Ogre::Quaternion::IDENTITY.z,
			Ogre::Quaternion::IDENTITY.w)
		);

	terrainRigidBody = new SimpleRigidBody( mSceneMgr, TumbuEnums::TERRAIN );

	terrainRigidBody->setOgreBulletRigidBody( defaultTerrainBody );
	terrainRigidBody->shape = mTerrainShape;
	terrainRigidBody->node = pTerrainNode;
}
//-------------------------------------------------------------------------------------
void Demo::createSimpleTerrain(void){
}
//-------------------------------------------------------------------------------------
void Demo::createMainCharacter(void){
	mainChar = new Character(physicWorld, "hero", soundManager);
	tumbu->getRoot()->addFrameListener( mainChar );
	tumbu->addKeyListener( mainChar, "heroKeyListener" );
	tumbu->addMouseListener( mainChar, "heroMouseListener" );
	tumbu->addJoystickListener( mainChar, "heroJoyListener" );

	ConfigNode* cfg;
	cfg = ConfigScriptLoader::getSingleton().getConfigScript( "demo", mainChar->robotName );

	/** CRIA UM NÓ PARA CADA PARTE DO CORPO */
	mainChar->buildNodes();

	// DevTest -hero=robotNNN: the hero wears all five parts of one set (to look at a given robot).
	const Ogre::String &heroSet = DevTest::getHeroSet();
	mainChar->addPart( HEAD, heroSet.empty() ? cfg->findChild("head")->getValue(0) : heroSet );
	mainChar->addPart( BODY, heroSet.empty() ? cfg->findChild("body")->getValue(0) : heroSet );
	mainChar->addPart( RIGHT_ARM, heroSet.empty() ? cfg->findChild("rightArm")->getValue(0) : heroSet );
	mainChar->addPart( LEFT_ARM, heroSet.empty() ? cfg->findChild("leftArm")->getValue(0) : heroSet );
	mainChar->addPart( LEGS, heroSet.empty() ? cfg->findChild("legs")->getValue(0) : heroSet );

	mainChar->head		= mainChar->headList[0];
	mainChar->body		= mainChar->bodyList[0];
	mainChar->rightArm	= mainChar->rightArmList[0];
	mainChar->leftArm	= mainChar->leftArmList[0];
	mainChar->legs		= mainChar->legsList[0];

	mainChar->buildCharacter();

	gui->setHero( mainChar );
}
//-------------------------------------------------------------------------------------
void Demo::createEnemyCharacter(void){
	enemy = new CharacterEnemy(physicWorld, "enemy", soundManager);
	tumbu->getRoot()->addFrameListener( enemy );

	ConfigNode* cfg;
	cfg = ConfigScriptLoader::getSingleton().getConfigScript( "demo", enemy->robotName );

	/** CRIA UM NÓ PARA CADA PARTE DO CORPO */
	enemy->buildNodes();

	enemy->addPart( HEAD, cfg->findChild("head")->getValue(0) );
	enemy->addPart( BODY, cfg->findChild("body")->getValue(0) );
	enemy->addPart( RIGHT_ARM, cfg->findChild("rightArm")->getValue(0) );
	enemy->addPart( LEFT_ARM, cfg->findChild("leftArm")->getValue(0) );
	enemy->addPart( LEGS, cfg->findChild("legs")->getValue(0) );

	enemy->head		= enemy->headList[0];
	enemy->body		= enemy->bodyList[0];
	enemy->rightArm	= enemy->rightArmList[0];
	enemy->leftArm	= enemy->leftArmList[0];
	enemy->legs		= enemy->legsList[0];

	enemy->buildCharacter();

	gui->setEnemy( enemy );

	numberOfenemies++;

	tumbu->getAIManager()->createDefensiveRobotAI( mainChar, enemy );
}
//-------------------------------------------------------------------------------------
void Demo::startInitialConversation(void){
	gui->addConversation("Your Robot", "robot001", "Hello...");
	gui->addConversation("Enemy Robot", "robot002", "Hello...");
	gui->addConversation("Your Robot", "robot001", "Welcome to Tumbu...");
	gui->addConversation("Your Robot", "robot001", "I'll be your robot in this demo...");
	gui->addConversation("Your Robot", "robot001", "Feel free to press <ESC> to see game options...");
	gui->addConversation("Enemy Robot", "robot002", "Good Luck.");

	gui->addShowPart( "Head", "Your Head:", mainChar->head);
	gui->addShowPart( "Body", "Your body:", mainChar->body);
	gui->addShowPart( "Left Arm", "Your left arm:", mainChar->leftArm);
	gui->addShowPart( "Right Arm", "Your right arm:", mainChar->rightArm);
	gui->addShowPart( "Legs", "Your legs:", mainChar->legs);

	gui->addConfirm( "Tutorial Mode?", "Do you like to start the tutorial mode?", &GUI::startTutorialMode, &GUI::dialogCancel );

	gui->showNextDialog(true);

}
//-------------------------------------------------------------------------------------
void Demo::waitForTerrainLighting(void){
	// The terrain computes its light map and composite map in a background thread after loading. Until the
	// result arrives (a few seconds) the grass renders dark, so wait for it on the loading screen. Frames must
	// be rendered meanwhile: the work queue hands the result over on the main thread.
	Ogre::TerrainGroup* terrain = mLoader->getTerrainGroup();
	if( terrain == NULL ){
		return;
	}
	gui->startLoad( "Lighting Terrain..." );
	Ogre::Timer timer;
	while( terrain->isDerivedDataUpdateInProgress() && timer.getMilliseconds() < 30000 ){
		tumbu->renderOneFrame();
	}
	if( terrain->isDerivedDataUpdateInProgress() ){
		Ogre::LogManager::getSingleton().logWarning( "[Demo] terrain lighting still updating after 30 s; continuing" );
	}else{
		Ogre::LogManager::getSingleton().logMessage( "[Demo] terrain lighting ready after " + Ogre::StringConverter::toString( timer.getMilliseconds() ) + " ms" );
	}
}
//-------------------------------------------------------------------------------------
void Demo::initialiseGameResources(void){
	// Ogre 14 initialises every group at startup; this only covers groups that are still pending.
	Ogre::ResourceGroupManager &rgm = Ogre::ResourceGroupManager::getSingleton();
	const char* groups[] = { "Plants", "TerrainTextures", "Game" };
	for( size_t i = 0; i < sizeof(groups) / sizeof(groups[0]); i++ ){
		if( rgm.resourceGroupExists( groups[i] ) && !rgm.isResourceGroupInitialised( groups[i] ) ){
			rgm.initialiseResourceGroup( groups[i] );
		}
	}
}
//-------------------------------------------------------------------------------------
void Demo::setupCamera(void){
	camera = new Camera(tumbu->mCamera, mainChar->robotNode);
	tumbu->getRoot()->addFrameListener( camera );
	tumbu->addKeyListener( camera, "CameraKeyListener" );
	tumbu->addMouseListener( camera, "CameraMouseListener" );
	tumbu->addJoystickListener( camera, "CameraKeyListener" );
	mainChar->setActiveCameraNode(camera->mCameraNode);
	enemy->setActiveCameraNode(camera->mCameraNode);

	// Free camera to look around the arena (F); camera.object flyCamera 0 turns it off.
	if( FlyCamera::isEnabled() ){
		flyCamera = new FlyCamera( tumbu->mCamera );
		tumbu->getRoot()->addFrameListener( flyCamera );
		tumbu->addKeyListener( flyCamera, "FlyCameraKeyListener" );
		tumbu->addMouseListener( flyCamera, "FlyCameraMouseListener" );
	}
}
//-------------------------------------------------------------------------------------
void Demo::createLightEffects(void){
	// Tone mapping and grading of the final image; the lighting follows the clock (lighting.object).
	lighting->enablePostProcessing( tumbu->mWindow->getViewport( 0 ) );
	tumbu->getRoot()->addFrameListener( lighting );

	// Special-attack effects: energy lights, screen flash, camera shake and hit-stop (effects.object).
	effects = new EffectsManager( mSceneMgr );
	tumbu->getRoot()->addFrameListener( effects );

	// A depth shadow map that the robot and arena shaders sample themselves (TumbuToon.h). Every object
	// also shadows itself (a robot's arm on its body, the coliseum walls on the floor).
	mSceneMgr->setShadowTextureSettings( tumbu->getShadowTextureSize(), tumbu->getShadowTextureCount(), Ogre::PF_DEPTH32F );
	mSceneMgr->setShadowTextureSelfShadow( true );
	mSceneMgr->setShadowDirectionalLightExtrusionDistance( 100 );
	mSceneMgr->setShadowFarDistance(tumbu->getShadowFarDistance());
	mSceneMgr->setShadowTechnique(tumbu->getShadowTechnique());
	// Fit the shadow map to what the camera sees instead of a fixed box around the light.
	mSceneMgr->setShadowCameraSetup( Ogre::FocusedShadowCameraSetup::create() );
	// The sun, ambient light and toon shading values; after the shadow settings, which it passes to the shaders.
	lighting->setSun( mSceneMgr->getLight("skyXSpotLight") );
}
//-------------------------------------------------------------------------------------
void Demo::createSky(){
	sky = Sky::getInstance();
	sky->setClock( tumbu->getClock() );
	// The visible sun (or moon) follows the lighting rig.
	lighting->setSky( sky );
	tumbu->getRoot()->addFrameListener( sky );

	// DevTest -sky=0|1 overrides the option for one run (to compare Caelum with the skydome).
	switch( DevTest::getSky() >= 0 ? DevTest::getSky() : tumbu->getSkyQuality() ){
	case 0:
		sky->skyLowQuality();
		break;
	case 1:
		sky->skyHighQuality( tumbu->mCamera );
		break;
	default:
		sky->skyLowQuality();
		break;
	}
}
//-------------------------------------------------------------------------------------
void Demo::verifyDeaths(void){
	if( mainChar->hp <= 0) {
		heroDie();
	}else if( enemy->hp <=0 ){
		enemyDie();
	}
}
//-------------------------------------------------------------------------------------
void Demo::heroDie(void){
	gui->addAlert("You Die!", "Game Over\nPress Ok...", &GUI::gameOverOk);
	gui->showNextDialog(true);
}
//-------------------------------------------------------------------------------------
void Demo::enemyDie(void){
	tumbu->setGameState( TumbuEnums::LOADING );
	SoundManager::getInstance()->print = true;
	SoundManager::getInstance()->printAllSounds();

	if( numberOfenemies == TOTAL_ENEMIES-1){
		gui->addAlert("CONGRATULATIONS!", "Impressive!\nYou Defeat All enemies\nThanks For play\nPress Ok to continue...", &GUI::gameOverOk);
		gui->showNextDialog(true);
		
		tumbu->setGameState( TumbuEnums::START_SCREEN );
	}else{
		Part *prizePart;
		int randomPart = rand() % 5;
		
		mainChar->enemyList.clear();
		prizePart = enemy->removePart(randomPart);

		gui->addShowPart("Congratulations, You Win!", "You win a new robot piece! \nCheck the Menu <ESC> -> Inventory.", prizePart);

		tumbu->getAIManager()->reset();
		
		SoundManager::getInstance()->printAllSounds();

		enemy->enemyList.clear();
		tumbu->getRoot()->removeFrameListener( enemy );
		
		tumbu->renderOneFrame();

		mainChar->addPart( prizePart );
		mainChar->addHpInPercent( 25 );
		mainChar->addApInPercent( 40 );

		mainChar->isAttacking = false;
		mainChar->resetAllAnimations();
		mainChar->stopAllSkills();

		SoundManager::getInstance()->printAllSounds();

		delete enemy;

		createEnemyCharacter();

		mainChar->addEnemy( enemy );
		enemy->addEnemy( mainChar );

		Ogre::String conversationFace = Ogre::StringUtil::replaceAll(enemy->head->setName ,"head", "robot");
		gui->addConversation("Enemy", conversationFace, "Now I'll defeat you");

		SoundManager::getInstance()->printAllSounds();

		gui->showNextDialog(true);

		SoundManager::getInstance()->printAllSounds();

		tumbu->setGameState( TumbuEnums::IN_DIALOG );
	}
}
//-------------------------------------------------------------------------------------
void Demo::setCamera( Camera* _camera ){
	camera = _camera;
}
//-------------------------------------------------------------------------------------
Camera* Demo::getCamera(){
	return camera;
}
//-------------------------------------------------------------------------------------
void Demo::setPhysicWorld( Physics::DynamicsWorld* _physicWorld ){
	physicWorld = _physicWorld;
}
//-------------------------------------------------------------------------------------
Physics::DynamicsWorld* Demo::getPhysicWorld(){
	return physicWorld;
}
//-------------------------------------------------------------------------------------
