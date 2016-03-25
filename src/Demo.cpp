#include "Demo.h"
//-------------------------------------------------------------------------------------
Demo::Demo(){
	gui				= GUI::getInstance();
	soundManager	= SoundManager::getInstance();
	tumbu			= TUMBU::getInstance();
	mSceneMgr		= tumbu->mSceneMgr;

	arenaRigidBody				= NULL;
	backGroundSound				= NULL;
	colliseumRigidBody			= NULL;
#ifdef _DEBUG
	debugDrawer					= NULL;
	debugDrawerNode				= NULL;
#endif
	enemy						= NULL;
	mainChar					= NULL;
	tutorial					= NULL;

	terrainRigidBody			= NULL;
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
	Ogre::LogManager::getSingletonPtr()->logMessage("\tDeleting Sky...");
	if( sky != NULL ){
		tumbu->getRoot()->removeFrameListener( sky );
		delete sky;
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
#ifdef _DEBUG
	if( debugDrawerNode != NULL && debugDrawer != NULL ){
		debugDrawerNode->removeAndDestroyAllChildren();
		mSceneMgr->destroySceneNode( debugDrawerNode );
		debugDrawer->setDrawWireframe(false);
		getPhysicWorld()->setShowDebugShapes(false);
		getPhysicWorld()->setShowDebugContactPoints(false);
		debugDrawer->clear();
		delete debugDrawer;
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
		delete pDataConvert;
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
	const Ogre::Real timeSinceLastFrame = evt.timeSinceLastFrame;

	switch( tumbu->getGameState() ){
	case TumbuEnums::PLAYING:
		getPhysicWorld()->stepSimulation(timeSinceLastFrame);
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
	btRigidBody *rigid,
				*rigid2;

	CollisionDetectionListener *listener1,
							   *listener2;

	std::string rigidName,
				rigid2Name;

	btVector3	
		collisionPointA,
		collisionPointB;

	getPhysicWorld()->getBulletCollisionWorld()->performDiscreteCollisionDetection();

	const unsigned int numManifolds = getPhysicWorld()->getBulletCollisionWorld()->getDispatcher()->getNumManifolds();
	for (unsigned int i = 0; i < numManifolds; i++){
		try{
			contactManifold =  getPhysicWorld()->getBulletCollisionWorld()->getDispatcher()->getManifoldByIndexInternal(i);
	
			rigid = static_cast<btRigidBody*>(contactManifold->getBody0());
			rigid2 = static_cast<btRigidBody*>(contactManifold->getBody1());

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
	/*
					cout << " listener 1 =  " << listener1->rigidBodyName << " 2 - " << listener2->rigidBodyName << " contacts = " << numContacts << endl;

					for (int j=0;j<numContacts;j++){
						btManifoldPoint& pt = contactManifold->getContactPoint(j);
						if (pt.getDistance()<0.f)
						{
							const btVector3& ptA = pt.getPositionWorldOnA();
							const btVector3& ptB = pt.getPositionWorldOnB();
							const btVector3& normalOnB = pt.m_normalWorldOnB;
						}
					}
	*/
					listener1->collisionPosition = OgreBulletCollisions::BtOgreConverter::to( contactManifold->getContactPoint(0).getPositionWorldOnA() );
					listener2->collisionPosition = OgreBulletCollisions::BtOgreConverter::to( contactManifold->getContactPoint(0).getPositionWorldOnB() );

					listener1->collisionPosition = OgreBulletCollisions::BtOgreConverter::to( collisionPointA );
					listener2->collisionPosition = OgreBulletCollisions::BtOgreConverter::to( collisionPointB );

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
		}catch(...){}
	}
}
//-------------------------------------------------------------------------------------
void Demo::initializeDemo(){

	printf("Carregando Scripts\n");
	gui->startLoad("Loading Scripts...");
	initialiseGameResources();

	printf("Carregando Cena\n");
	gui->startLoad("Loading Scene...");
	tumbu->renderOneFrame();
	mLoader = new DotSceneLoader();
	tumbu->renderOneFrame();
	mLoader->parseDotScene("Arena.scene", "General", mSceneMgr);

	printf("Carregando Cena\n");
	gui->startLoad("Loading Scene...");
	tumbu->renderOneFrame();
	demoScene = mSceneMgr->getSceneNode("demoScene");
	tumbu->renderOneFrame();
	tumbu->setActiveSceneNode( demoScene );
	tumbu->renderOneFrame();

	printf("Inicializando luzes\n");
	gui->startLoad("Loading Lights...");
	tumbu->renderOneFrame();
	createLightEffects();
 
	printf("Inicializando coisas fisica\n");
	gui->startLoad("Loading Physics...");
	tumbu->renderOneFrame();
	initializePhysicsStuff();

	printf("Inicializando SKY\n");
	gui->startLoad("Loading Sky...");
	tumbu->renderOneFrame();
	createSky();


	printf("Inicializando terreno\n");
	gui->startLoad("Loading Terrain...");
	tumbu->renderOneFrame();
	createTerrainPhysic();

	printf("Inicializando arena\n");
	gui->startLoad("Loading Terrain...");
	tumbu->renderOneFrame();
	createArena();

	printf("Criando personagem principal\n");
	gui->startLoad("Loading Character...");
	tumbu->renderOneFrame();
	createMainCharacter();

	printf("Criando personagem outro\n");
	gui->startLoad("Loading Character...");
	tumbu->renderOneFrame();
	createEnemyCharacter();

	printf("Inicializando Camera\n");
	gui->startLoad("Loading Camera...");
	tumbu->renderOneFrame();
	setupCamera();
	
	backGroundSound = soundManager->createSound( "backgroundSound", "battle_music.ogg", Ogre::Vector3(0,0,0), true, false);
	backGroundSound->play();

	gui->stopLoad();

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
	physicWorld = new OgreBulletDynamics::DynamicsWorld( mSceneMgr, 
		Ogre::AxisAlignedBox( Ogre::Vector3 (-100, -100, -100), Ogre::Vector3 (100,  100,  100) ), 
		Ogre::Vector3(0,-9.81f,0), true, true, 32768
	);

	#ifdef _DEBUG
	if( debugDrawer == NULL ){
		debugDrawer = new OgreBulletCollisions::DebugDrawer();
		debugDrawer->setDrawWireframe(true);
		getPhysicWorld()->setDebugDrawer(debugDrawer);
		getPhysicWorld()->setShowDebugShapes(true);
		getPhysicWorld()->setShowDebugContactPoints(true);
		debugDrawerNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("debugDrawer", Ogre::Vector3::ZERO);
		debugDrawerNode->attachObject(static_cast <Ogre::SimpleRenderable *> (debugDrawer));
	}
	#endif
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
	OgreBulletCollisions::CollisionShape* mTerrainShape;
	OgreBulletDynamics::RigidBody* defaultTerrainBody;

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

	mTerrainShape = new OgreBulletCollisions:: HeightmapCollisionShape(
		pTerrain->getSize(), 
		pTerrain->getSize(),	
		localScaling,
		pDataConvert,
		pTerrain->getMaxHeight(), 
		pTerrain->getMinHeight(), 
		true);

	defaultTerrainBody = new OgreBulletDynamics::RigidBody(
		"Terrain", 
		getPhysicWorld());

	Ogre::Vector3 position(
		terrainPosition.x,
		terrainPosition.y + (pTerrain->getMaxHeight()-1.5f)/2,
		terrainPosition.z
	);

	/*
	cout << "MAX HEIGHT " << pTerrain->getMaxHeight() << endl;
	cout << "MIN HEIGHT " << pTerrain->getMinHeight() << endl;
	cout << "Y: " << position.y << endl;
	*/

	pTerrainNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("terrain_node");
	defaultTerrainBody->setStaticShape (pTerrainNode, mTerrainShape, terrainBodyRestitution, terrainBodyFriction, position);

	defaultTerrainBody->getBulletRigidBody()->getWorldTransform().setRotation(
		btQuaternion(
			Ogre::Quaternion::IDENTITY.x,
			Ogre::Quaternion::IDENTITY.y,
			Ogre::Quaternion::IDENTITY.z,
			Ogre::Quaternion::IDENTITY.w)
		);

	terrainRigidBody = new SimpleRigidBody( mSceneMgr, TumbuEnums::PhysicObjectTag::TERRAIN );

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

	mainChar->addPart( HEAD, cfg->findChild("head")->getValue(0) );
	mainChar->addPart( BODY, cfg->findChild("body")->getValue(0) );
	mainChar->addPart( RIGHT_ARM, cfg->findChild("rightArm")->getValue(0) );
	mainChar->addPart( LEFT_ARM, cfg->findChild("leftArm")->getValue(0) );
	mainChar->addPart( LEGS, cfg->findChild("legs")->getValue(0) );

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
void Demo::initialiseGameResources(void){
	if ( !Ogre::ResourceGroupManager::getSingleton().isResourceGroupInitialised("EditorResources") ){
		Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("EditorResources");
	}

	if ( !Ogre::ResourceGroupManager::getSingleton().isResourceGroupInitialised("Brushes") ){
		Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Brushes");
	}

	if ( !Ogre::ResourceGroupManager::getSingleton().isResourceGroupInitialised("Plants") ){
		Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Plants");
	}

	if ( !Ogre::ResourceGroupManager::getSingleton().isResourceGroupInitialised("TerrainTextures") ){
		Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("TerrainTextures");
	}
	
	if ( !Ogre::ResourceGroupManager::getSingleton().isResourceGroupInitialised("Game") ){
		Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Game");
	}

	if( tumbu->getSkyQuality() == 1 ){
		if ( !Ogre::ResourceGroupManager::getSingleton().isResourceGroupInitialised("SkyX") == 1 ){
			Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("SkyX");
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
}
//-------------------------------------------------------------------------------------
void Demo::createLightEffects(void){
	mSceneMgr->setAmbientLight(Ogre::ColourValue(0.4f, 0.4f, 0.4f));
	
	// Set Shadow Properties
	mSceneMgr->setShadowTechnique(tumbu->getShadowTechnique());
	mSceneMgr->setShadowColour(tumbu->getShadowColor());
	mSceneMgr->setShadowFarDistance(tumbu->getShadowFarDistance());
	mSceneMgr->setShadowTextureSize(tumbu->getShadowTextureSize());
	mSceneMgr->setShadowTextureCount(tumbu->getShadowTextureCount());
}
//-------------------------------------------------------------------------------------
void Demo::createSky(){
	sky = Sky::getInstance();
	sky->setClock( tumbu->getClock() );
	tumbu->getRoot()->addFrameListener( sky );

	switch( tumbu->getSkyQuality() ){
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
void Demo::setPhysicWorld( OgreBulletDynamics::DynamicsWorld* _physicWorld ){
	physicWorld = _physicWorld;
}
//-------------------------------------------------------------------------------------
OgreBulletDynamics::DynamicsWorld* Demo::getPhysicWorld(){
	return physicWorld;
}
//-------------------------------------------------------------------------------------