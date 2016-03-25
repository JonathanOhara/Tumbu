#include "BaseApplication.h"
#include "GUI.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include "../res/resource.h"
#endif
//-------------------------------------------------------------------------------------
BaseApplication::BaseApplication(void)
	: mRoot(0),
	mCamera(0),
	mSceneMgr(0),
	mWindow(0),
	mResourcesCfg(Ogre::StringUtil::BLANK),
	mPluginsCfg(Ogre::StringUtil::BLANK),
	mTrayMgr(0),
	mCameraMan(0),
	mDetailsPanel(0),
	mCursorWasVisible(false),
	mShutDown(false),
	mInputManager(0),
	mMouse(0),
	mKeyboard(0)
{
}

//-------------------------------------------------------------------------------------
BaseApplication::~BaseApplication(void){
	if (mTrayMgr) delete mTrayMgr;
	if (mCameraMan) delete mCameraMan;

	mKeyListeners.clear();
	mMouseListeners.clear();


	//Remove ourself as a Window listener
	Ogre::WindowEventUtilities::removeWindowEventListener(mWindow, this);
	windowClosed(mWindow);
	delete ConfigScriptLoader::getSingletonPtr();
	delete mRoot;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::configure(void){
	// Show the configuration dialog and initialise the system
	// You can skip this and use root.restoreConfig() to load configuration
	// settings if you were sure there are valid ones saved in ogre.cfg
	if(mRoot->restoreConfig() || mRoot->showConfigDialog()){
		// If returned true, user clicked OK so initialise
		// Here we choose to let the system create a default rendering window by passing 'true'
		mWindow = mRoot->initialise(true, "TUMBU");

		// Let's add a nice window icon
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		HWND hwnd;
		mWindow->getCustomAttribute("WINDOW", (void*)&hwnd);
		LONG iconID   = (LONG)LoadIcon( GetModuleHandle(0), MAKEINTRESOURCE(IDI_APPICON) );
		SetClassLong( hwnd, GCL_HICON, iconID );
#endif
		return true;
	}
	else{
		return false;
	}
}
//-------------------------------------------------------------------------------------
void BaseApplication::chooseSceneManager(void){
	// Get the SceneManager, in this case a generic one
	mSceneMgr = mRoot->createSceneManager(Ogre::ST_GENERIC,"SceneManager");
}
//-------------------------------------------------------------------------------------
void BaseApplication::createCamera(void){
	// Create the camera
	mCamera = mSceneMgr->createCamera("PlayerCam");

	// Position
	mCamera->setPosition(Ogre::Vector3( 0, 2, 4));

	mCamera->setNearClipDistance(0.1);

	mCameraMan = new OgreBites::SdkCameraMan(mCamera);   // create a default camera controller
}
//-------------------------------------------------------------------------------------
void BaseApplication::createFrameListener(void){
	Ogre::LogManager::getSingletonPtr()->logMessage("*** Initializing OIS ***");
	OIS::ParamList pl;
	size_t windowHnd = 0;
	std::ostringstream windowHndStr;

	mWindow->getCustomAttribute("WINDOW", &windowHnd);
	windowHndStr << windowHnd;
	pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

	mInputManager = OIS::InputManager::createInputSystem( pl );

	mKeyboard = static_cast<OIS::Keyboard*>(mInputManager->createInputObject( OIS::OISKeyboard, true ));
	mMouse = static_cast<OIS::Mouse*>(mInputManager->createInputObject( OIS::OISMouse, true ));

	mMouse->setEventCallback(this);
	mKeyboard->setEventCallback(this);
    
	if (mInputManager->getNumberOfDevices(OIS::OISJoyStick) > 0) {
        mJoysticks.resize( mInputManager->getNumberOfDevices(OIS::OISJoyStick) );
 
        std::vector<OIS::JoyStick*>::iterator itJoystick    = mJoysticks.begin();
        std::vector<OIS::JoyStick*>::iterator itJoystickEnd = mJoysticks.end();
        for(; itJoystick != itJoystickEnd; ++itJoystick ) {
            (*itJoystick) = static_cast<OIS::JoyStick*>( mInputManager->createInputObject( OIS::OISJoyStick, true ) );
            (*itJoystick)->setEventCallback( this );
        }
    }

	//Set initial mouse clipping size
	windowResized(mWindow);

	//Register as a Window listener
	Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);

	mTrayMgr = new OgreBites::SdkTrayManager("InterfaceName", mWindow, mMouse, this);

#ifdef _DEBUG
	mTrayMgr->showFrameStats(OgreBites::TL_BOTTOMLEFT);
#endif
	mTrayMgr->hideCursor();
	
	mTrayMgr->toggleAdvancedFrameStats();
#ifdef _DEBUG
	mTrayMgr->toggleAdvancedFrameStats();
	// create a params panel for displaying sample details
	Ogre::StringVector items;
	items.push_back("cam.pX");
	items.push_back("cam.pY");
	items.push_back("cam.pZ");
	items.push_back("");
	items.push_back("cam.oW");
	items.push_back("cam.oX");
	items.push_back("cam.oY");
	items.push_back("cam.oZ");
	items.push_back("");
	items.push_back("Filtering");
	items.push_back("Poly Mode");

	mDetailsPanel = mTrayMgr->createParamsPanel(OgreBites::TL_NONE, "DetailsPanel", 200, items);
	mDetailsPanel->setParamValue(9, "Bilinear");
	mDetailsPanel->setParamValue(10, "Solid");
	mDetailsPanel->hide();
#endif
	mRoot->addFrameListener(this);
}
//-------------------------------------------------------------------------------------
void BaseApplication::destroyScene(void){
}
//-------------------------------------------------------------------------------------
void BaseApplication::createViewports(void){
	// Create one viewport, entire window
	mViewport = mWindow->addViewport(mCamera);
	mViewport->setBackgroundColour(Ogre::ColourValue(0,0,0));

	// Alter the camera aspect ratio to match the viewport
	mCamera->setAspectRatio(Ogre::Real(mViewport->getActualWidth()) / Ogre::Real(mViewport->getActualHeight()));
}
//-------------------------------------------------------------------------------------
void BaseApplication::setupResources(void){
	new ConfigScriptLoader();

	// Load resource paths from config file
	Ogre::ConfigFile cf;
	cf.load(mResourcesCfg);

	// Go through all sections & settings in the file
	Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

	Ogre::String secName, typeName, archName;
	while (seci.hasMoreElements())
	{
		secName = seci.peekNextKey();
		Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
		Ogre::ConfigFile::SettingsMultiMap::iterator i;
		for (i = settings->begin(); i != settings->end(); ++i)
		{
			typeName = i->first;
			archName = i->second;
			Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
				archName, typeName, secName);
		}
	}
}
//-------------------------------------------------------------------------------------
void BaseApplication::createResourceListener(void){

}
//-------------------------------------------------------------------------------------
void BaseApplication::loadResources(void){
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Essential");
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("General");

	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("imagesets");
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Fonts");
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Schemes");
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("LookNFeel");
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Layouts");
}
//-------------------------------------------------------------------------------------
void BaseApplication::go(void){
#ifdef _DEBUG
	mResourcesCfg = "tumbu_d.cfg";
	mPluginsCfg = "plugins_d.cfg";
#else
	mResourcesCfg = "tumbu.cfg";
	mPluginsCfg = "plugins.cfg";
#endif

	if (!setup())
		return;

	mRoot->startRendering();

	destroyScene();
}
//-------------------------------------------------------------------------------------
bool BaseApplication::setup(void){
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	
	char logPathChar[250];

	ExpandEnvironmentStrings("%userprofile%",logPathChar,250);

	workPath = Ogre::String(logPathChar).append("\\Tumbu\\");

	Ogre::String logPath(workPath);
	Ogre::String cfgPath(workPath);
	
	logPath.append("ogre.log");
	cfgPath.append("ogre.cfg");

	mRoot = new Ogre::Root(mPluginsCfg, cfgPath, logPath);

	Ogre::LogManager::getSingletonPtr()->logMessage("Created Ogre with Work dir in " + workPath);
#endif
#if OGRE_PLATFORM != OGRE_PLATFORM_WIN32

	mRoot = new Ogre::Root(mPluginsCfg);
#endif
	setupResources();

	bool carryOn = configure();
	if (!carryOn) return false;

	chooseSceneManager();
	createCamera();
	createViewports();

	// Set default mipmap level (NB some APIs ignore this)
	Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

	// Create any resource listeners (for loading screens)
	createResourceListener();
	// Load resources
	loadResources();

	createFrameListener();

	// Create the scene
	createScene();

	return true;
};
//-------------------------------------------------------------------------------------
bool BaseApplication::frameRenderingQueued(const Ogre::FrameEvent& evt){	
//	cout << "BaseApp" << endl;
	if(mWindow->isClosed())
		return false;

	if(mShutDown)
		return false;

	mKeyboard->capture();
	mMouse->capture();

    if( mJoysticks.size() > 0 ) {
        std::vector<OIS::JoyStick*>::iterator itJoystick    = mJoysticks.begin();
        std::vector<OIS::JoyStick*>::iterator itJoystickEnd = mJoysticks.end();
        for(; itJoystick != itJoystickEnd; ++itJoystick ) {
            (*itJoystick)->capture();
        }
    }

	mTrayMgr->frameRenderingQueued(evt);

#ifdef _DEBUG
	if (!mTrayMgr->isDialogVisible()){
		mCameraMan->frameRenderingQueued(evt);

		if (mDetailsPanel->isVisible()){
			mDetailsPanel->setParamValue(0, Ogre::StringConverter::toString(mCamera->getDerivedPosition().x));
			mDetailsPanel->setParamValue(1, Ogre::StringConverter::toString(mCamera->getDerivedPosition().y));
			mDetailsPanel->setParamValue(2, Ogre::StringConverter::toString(mCamera->getDerivedPosition().z));
			mDetailsPanel->setParamValue(4, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().w));
			mDetailsPanel->setParamValue(5, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().x));
			mDetailsPanel->setParamValue(6, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().y));
			mDetailsPanel->setParamValue(7, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().z));
		}

	}
#endif
	return true;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::keyPressed( const OIS::KeyEvent &arg ){
	if (mTrayMgr->isDialogVisible()) return true; 

#ifdef _DEBUG
	if (arg.key == OIS::KC_G){ //Detalhes da Camera
		if (mDetailsPanel->getTrayLocation() == OgreBites::TL_NONE)
		{
			mTrayMgr->moveWidgetToTray(mDetailsPanel, OgreBites::TL_TOPRIGHT, 0);
			mDetailsPanel->show();
		}
		else
		{
			mTrayMgr->removeWidgetFromTray(mDetailsPanel);
			mDetailsPanel->hide();
		}
	}else if (arg.key == OIS::KC_R){ //Tipo de Renderizacao
		Ogre::String newVal;
		Ogre::PolygonMode pm;

		switch (mCamera->getPolygonMode())
		{
		case Ogre::PM_SOLID:
			newVal = "Wireframe";
			pm = Ogre::PM_WIREFRAME;
			break;
		case Ogre::PM_WIREFRAME:
			newVal = "Points";
			pm = Ogre::PM_POINTS;
			break;
		default:
			newVal = "Solid";
			pm = Ogre::PM_SOLID;
		}

		mCamera->setPolygonMode(pm);
		mDetailsPanel->setParamValue(10, newVal);
	}
	else if(arg.key == OIS::KC_F5){ //Recarregar Texturas
		Ogre::TextureManager::getSingleton().reloadAll();
	}
	else if (arg.key == OIS::KC_SYSRQ){ //Tirar Foto
		mWindow->writeContentsToTimestampedFile("screenshot", ".jpg");
	}
#endif

    std::map<std::string, OIS::KeyListener*>::iterator itKeyListener = mKeyListeners.begin();
    std::map<std::string, OIS::KeyListener*>::iterator itKeyListenerEnd = mKeyListeners.end();
    for(; itKeyListener != itKeyListenerEnd; ++itKeyListener ) {
        itKeyListener->second->keyPressed( arg );
    }

	return true;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::keyReleased( const OIS::KeyEvent &arg ){
	if (mTrayMgr->isDialogVisible()) return true;

    std::map<std::string, OIS::KeyListener*>::iterator itKeyListener    = mKeyListeners.begin();
    std::map<std::string, OIS::KeyListener*>::iterator itKeyListenerEnd = mKeyListeners.end();
    for(; itKeyListener != itKeyListenerEnd; ++itKeyListener ) {
        itKeyListener->second->keyReleased( arg );
    }
    return true;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::mouseMoved( const OIS::MouseEvent &arg ){
//	if (mTrayMgr->injectMouseMove(arg)) return true;

    std::map<std::string, OIS::MouseListener*>::iterator itMouseListener    = mMouseListeners.begin();
    std::map<std::string, OIS::MouseListener*>::iterator itMouseListenerEnd = mMouseListeners.end();
    for(; itMouseListener != itMouseListenerEnd; ++itMouseListener ) {
        itMouseListener->second->mouseMoved( arg );
    }
	return true;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id ){
//	if (mTrayMgr->injectMouseDown(arg, id)) return true;

	std::map<std::string, OIS::MouseListener*>::iterator itMouseListener    = mMouseListeners.begin();
    std::map<std::string, OIS::MouseListener*>::iterator itMouseListenerEnd = mMouseListeners.end();
    for(; itMouseListener != itMouseListenerEnd; ++itMouseListener ) {
        itMouseListener->second->mousePressed( arg, id );
    }
    return true;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id ){
//	if (mTrayMgr->injectMouseUp(arg, id)) return true;

	std::map<std::string, OIS::MouseListener*>::iterator itMouseListener    = mMouseListeners.begin();
    std::map<std::string, OIS::MouseListener*>::iterator itMouseListenerEnd = mMouseListeners.end();

	for(; itMouseListener != itMouseListenerEnd; ++itMouseListener ) {
		itMouseListener->second->mouseReleased( arg, id );
    }

	
    return true;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::povMoved( const OIS::JoyStickEvent &e, int pov ) {
    std::map<std::string, OIS::JoyStickListener*>::iterator itJoystickListener    = mJoystickListeners.begin();
    std::map<std::string, OIS::JoyStickListener*>::iterator itJoystickListenerEnd = mJoystickListeners.end();
    for(; itJoystickListener != itJoystickListenerEnd; ++itJoystickListener ) {
        itJoystickListener->second->povMoved( e, pov );
    }
 
    return true;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::axisMoved( const OIS::JoyStickEvent &e, int axis ) {
    std::map<std::string, OIS::JoyStickListener*>::iterator itJoystickListener    = mJoystickListeners.begin();
    std::map<std::string, OIS::JoyStickListener*>::iterator itJoystickListenerEnd = mJoystickListeners.end();
    for(; itJoystickListener != itJoystickListenerEnd; ++itJoystickListener ) {
        itJoystickListener->second->axisMoved( e, axis );
    }
 
    return true;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::sliderMoved( const OIS::JoyStickEvent &e, int sliderID ) {
    std::map<std::string, OIS::JoyStickListener*>::iterator itJoystickListener    = mJoystickListeners.begin();
    std::map<std::string, OIS::JoyStickListener*>::iterator itJoystickListenerEnd = mJoystickListeners.end();
    for(; itJoystickListener != itJoystickListenerEnd; ++itJoystickListener ) {
        itJoystickListener->second->sliderMoved( e, sliderID );
    }
 
    return true;
}
//------------------------------------------------------------------------------------- 
bool BaseApplication::buttonPressed( const OIS::JoyStickEvent &e, int button ) {
    std::map<std::string, OIS::JoyStickListener*>::iterator itJoystickListener    = mJoystickListeners.begin();
    std::map<std::string, OIS::JoyStickListener*>::iterator itJoystickListenerEnd = mJoystickListeners.end();
    for(; itJoystickListener != itJoystickListenerEnd; ++itJoystickListener ) {
        itJoystickListener->second->buttonPressed( e, button );
    }
 
    return true;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::buttonReleased( const OIS::JoyStickEvent &e, int button ) {
    std::map<std::string, OIS::JoyStickListener*>::iterator itJoystickListener    = mJoystickListeners.begin();
    std::map<std::string, OIS::JoyStickListener*>::iterator itJoystickListenerEnd = mJoystickListeners.end();
    for(; itJoystickListener != itJoystickListenerEnd; ++itJoystickListener ) {
        itJoystickListener->second->buttonReleased( e, button );
    }
 
    return true;
}
//-------------------------------------------------------------------------------------
void BaseApplication::addKeyListener( OIS::KeyListener *keyListener, const std::string& instanceName ) {
    std::map<std::string, OIS::KeyListener*>::iterator itKeyListener;
	itKeyListener = mKeyListeners.find( instanceName );
    if( itKeyListener == mKeyListeners.end() ) {
        mKeyListeners[ instanceName ] = keyListener;
    }
}
//-------------------------------------------------------------------------------------
void BaseApplication::addMouseListener( OIS::MouseListener *mouseListener, const std::string& instanceName ) {
	std::map<std::string, OIS::MouseListener*>::iterator itMouseListener;
	itMouseListener = mMouseListeners.find( instanceName );
    if( itMouseListener == mMouseListeners.end() ) {
        mMouseListeners[ instanceName ] = mouseListener;
    }
}
//-------------------------------------------------------------------------------------
void BaseApplication::addCollisionDetectionListener( CollisionDetectionListener *collisionListener, const std::string& instanceName ) {
	std::map<std::string, CollisionDetectionListener*>::iterator itCollisionDetectionListener;
	itCollisionDetectionListener = mCollisionDetectionListeners.find( instanceName );
    if( itCollisionDetectionListener == mCollisionDetectionListeners.end() ) {
        mCollisionDetectionListeners[ instanceName ] = collisionListener;
    }else{
		cout << "Não adicionado = " << instanceName << endl;
	}
}
//-------------------------------------------------------------------------------------
void BaseApplication::addCollisionDetectionListener( CollisionDetectionListener *collisionListener ) {
	//O nome em branco é apenas para objetos de colisão com apenas 1 rigibody
	addCollisionDetectionListener( collisionListener, collisionListener->getOgreBulletRigidBody("")->getName() );
}
//-------------------------------------------------------------------------------------
void BaseApplication::addJoystickListener( OIS::JoyStickListener *joystickListener, const std::string& instanceName ) {
	if( mJoysticks.size() > 0 ) {
        itJoystickListener = mJoystickListeners.find( instanceName );

        if( itJoystickListener == mJoystickListeners.end() ) {
            mJoystickListeners[ instanceName ] = joystickListener;
        }
    }
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeKeyListener( const std::string& instanceName ) {
    std::map<std::string, OIS::KeyListener*>::iterator itKeyListener = mKeyListeners.find( instanceName );
    if( itKeyListener != mKeyListeners.end() ) {
        mKeyListeners.erase( itKeyListener );
    }
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeMouseListener( const std::string& instanceName ) {
    std::map<std::string, OIS::MouseListener*>::iterator itMouseListener = mMouseListeners.find( instanceName );
    if( itMouseListener != mMouseListeners.end() ) {
        mMouseListeners.erase( itMouseListener );
    }
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeKeyListener( OIS::KeyListener *keyListener ) {
    std::map<std::string, OIS::KeyListener*>::iterator itKeyListener    = mKeyListeners.begin();
    std::map<std::string, OIS::KeyListener*>::iterator itKeyListenerEnd = mKeyListeners.end();
    for(; itKeyListener != itKeyListenerEnd; ++itKeyListener ) {
        if( itKeyListener->second == keyListener ) {
            mKeyListeners.erase( itKeyListener );
            break;
        }
    }
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeMouseListener( OIS::MouseListener *mouseListener ) {
    std::map<std::string, OIS::MouseListener*>::iterator itMouseListener    = mMouseListeners.begin();
    std::map<std::string, OIS::MouseListener*>::iterator itMouseListenerEnd = mMouseListeners.end();
    for(; itMouseListener != itMouseListenerEnd; ++itMouseListener ) {
        if( itMouseListener->second == mouseListener ) {
            mMouseListeners.erase( itMouseListener );
            break;
        }
    }
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeJoystickListener( const std::string& instanceName ) {
    // Check if item exists
    itJoystickListener = mJoystickListeners.find( instanceName );
    if( itJoystickListener != mJoystickListeners.end() ) {
        mJoystickListeners.erase( itJoystickListener );
    }
    else {
        // Doesn't Exist
    }
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeJoystickListener( OIS::JoyStickListener *joystickListener ) {
    itJoystickListener    = mJoystickListeners.begin();
    itJoystickListenerEnd = mJoystickListeners.end();
    for(; itJoystickListener != itJoystickListenerEnd; ++itJoystickListener ) {
        if( itJoystickListener->second == joystickListener ) {
            mJoystickListeners.erase( itJoystickListener );
            break;
        }
    }
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeCollisionDetectionListener( const std::string& instanceName ) {
    // Check if item exists
    itCollisionDetectionListener = mCollisionDetectionListeners.find( instanceName );
    if( itCollisionDetectionListener != mCollisionDetectionListeners.end() ) {
        mCollisionDetectionListeners.erase( itCollisionDetectionListener );
    }
    else {
        // Doesn't Exist
    }
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeCollisionDetectionListener( CollisionDetectionListener *collisionDetectionListener ) {
    itCollisionDetectionListener    = mCollisionDetectionListeners.begin();
    itCollisionDetectionListenerEnd = mCollisionDetectionListeners.end();
    for(; itCollisionDetectionListener != itCollisionDetectionListenerEnd; ++itCollisionDetectionListener ) {
        if( itCollisionDetectionListener->second == collisionDetectionListener ) {
            mCollisionDetectionListeners.erase( itCollisionDetectionListener );
            break;
        }
    }
}
//-------------------------------------------------------------------------------------
CollisionDetectionListener* BaseApplication::getCollisionListenerByName( const std::string& instanceName ) {
	itCollisionDetectionListener = mCollisionDetectionListeners.find( instanceName );
	return ( itCollisionDetectionListener == mCollisionDetectionListeners.end() ) ? NULL : itCollisionDetectionListener->second;
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeAllListeners( void ) {
    mKeyListeners.clear();
    mMouseListeners.clear();
	mJoystickListeners.clear();
}
//------------------------------------------------------------------------------------- 
void BaseApplication::removeAllKeyListeners( void ) {
    mKeyListeners.clear();
}
 //-------------------------------------------------------------------------------------
void BaseApplication::removeAllMouseListeners( void ) {
    mMouseListeners.clear();
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeAllJoystickListeners( void ) {
    mJoystickListeners.clear();
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeAllCollisionDetectionListeners( void ) {
    mCollisionDetectionListeners.clear();
}
//-------------------------------------------------------------------------------------
void BaseApplication::windowResized(Ogre::RenderWindow* rw){
	unsigned int width, height, depth;
	int left, top;
	rw->getMetrics(width, height, depth, left, top);

	const OIS::MouseState &ms = mMouse->getMouseState();
	ms.width = width;
	ms.height = height;
}
//-------------------------------------------------------------------------------------
void BaseApplication::windowClosed(Ogre::RenderWindow* rw){
	if( rw == mWindow )	{
		if( mInputManager )	{
			mInputManager->destroyInputObject( mMouse );
			mInputManager->destroyInputObject( mKeyboard );

			OIS::InputManager::destroyInputSystem(mInputManager);
			mInputManager = 0;
		}
	}
}