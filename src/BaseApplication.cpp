#include "BaseApplication.h"
#include "GUI.h"
#include "DevTest.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../res/resource.h"
#endif
//-------------------------------------------------------------------------------------
/**
 * The viewport renders with the shader generator scheme, and the shader generator builds its technique from
 * the first technique with a fixed-function pass, i.e. a material's fallback. Materials that bring their own
 * shaders (robots, Caelum) must keep them, so for those this listener answers first with their shader
 * technique. Materials without shaders are left to the shader generator.
 */
class ProgrammableTechniqueResolver: public Ogre::MaterialManager::Listener{
public:
	Ogre::Technique* handleSchemeNotFound( unsigned short schemeIndex, const Ogre::String& schemeName,
		Ogre::Material* originalMaterial, unsigned short lodIndex, const Ogre::Renderable* rend ){
		for( Ogre::Technique* technique : originalMaterial->getSupportedTechniques() ){
			if( technique->getLodIndex() != lodIndex
				|| technique->getSchemeName() != Ogre::MaterialManager::DEFAULT_SCHEME_NAME ){
				continue;
			}
			// The best default technique decides: fully programmable → use it as is.
			for( Ogre::Pass* pass : technique->getPasses() ){
				if( !pass->isProgrammable() ){
					return NULL;
				}
			}
			return technique;
		}
		return NULL;
	}
};
static ProgrammableTechniqueResolver techniqueResolver;
//-------------------------------------------------------------------------------------
BaseApplication::BaseApplication(void)
	: OgreBites::ApplicationContext("TUMBU"),
	mSceneMgr(0),
	mRoot(0),
	mWindow(0),
	mCamera(0),
	mTrayMgr(0),
	mCameraMan(0),
	mDetailsPanel(0),
	mCameraNode(0),
	mViewport(0),
	mShutDown(false)
{
	// ogre.cfg, ogre.log and screenshots live in a per-user folder (same place as the 2011 game).
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	const char* home = getenv("USERPROFILE");
#else
	const char* home = getenv("HOME");
#endif
	workPath = Ogre::StringUtil::standardisePath( Ogre::String( home ? home : "." ) + "/Tumbu" );
	Ogre::FileSystemLayer::createDirectory( workPath );
	mFSLayer->setHomePath( workPath );
}
//-------------------------------------------------------------------------------------
BaseApplication::~BaseApplication(void){
	mInputListeners.clear();
}
//-------------------------------------------------------------------------------------
void BaseApplication::go(void){
	initApp();
	if( getRoot() != NULL && getRenderWindow() != NULL ){
		getRoot()->startRendering();
	}

	// Everything that talks to Ogre is released here, before closeApp() deletes the Root.
	destroyScene();
	delete mTrayMgr;
	mTrayMgr = NULL;
	delete mCameraMan;
	mCameraMan = NULL;
	delete ConfigScriptLoader::getSingletonPtr();
	if( Ogre::MaterialManager::getSingletonPtr() != NULL ){
		Ogre::MaterialManager::getSingleton().removeListener( &techniqueResolver, Ogre::MSN_SHADERGEN );
	}
	closeApp();
}
//-------------------------------------------------------------------------------------
void BaseApplication::setup(void){
	// Must exist before resources are parsed, so *.object scripts are picked up.
	new ConfigScriptLoader();

	OgreBites::ApplicationContext::setup();
	// Scheme-specific listeners are asked before the shader generator's generic one.
	Ogre::MaterialManager::getSingleton().addListener( &techniqueResolver, Ogre::MSN_SHADERGEN );
	mRoot = getRoot();
	mWindow = getRenderWindow();
	addInputListener( this );

	Ogre::LogManager::getSingletonPtr()->logMessage( "Created Ogre with work dir in " + workPath );

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	// Window icon from the executable's resources.
	HWND hwnd = 0;
	mWindow->getCustomAttribute( "WINDOW", &hwnd );
	if( hwnd ){
		HICON icon = LoadIcon( GetModuleHandle( 0 ), MAKEINTRESOURCE( IDI_APPICON ) );
		SendMessage( hwnd, WM_SETICON, ICON_BIG, (LPARAM) icon );
		SendMessage( hwnd, WM_SETICON, ICON_SMALL, (LPARAM) icon );
	}
#endif

	chooseSceneManager();
	createCamera();
	createViewports();
	Ogre::TextureManager::getSingleton().setDefaultNumMipmaps( 5 );
	createFrameListener();
	createScene();
}
//-------------------------------------------------------------------------------------
void BaseApplication::locateResources(void){
	// The robot shaders (Game) #include OgreUnifiedShader.h from OgreInternal. Only groups in the global pool
	// look for resources in other groups, so Game is created as one before resources.cfg fills it.
	Ogre::ResourceGroupManager::getSingleton().createResourceGroup( "Game", true );

	// resources.cfg (next to the executable) lists the game's resource groups and Ogre's own media.
	OgreBites::ApplicationContext::locateResources();
}
//-------------------------------------------------------------------------------------
void BaseApplication::loadResources(void){
	OgreBites::ApplicationContext::loadResources();
}
//-------------------------------------------------------------------------------------
void BaseApplication::chooseSceneManager(void){
	mSceneMgr = mRoot->createSceneManager( Ogre::SMT_DEFAULT, "SceneManager" );
	mSceneMgr->addRenderQueueListener( getOverlaySystem() );

	// Materials without shaders (arena, sky, particles, overlays) get them from the RTSS.
	Ogre::RTShader::ShaderGenerator::getSingleton().addSceneManager( mSceneMgr );
}
//-------------------------------------------------------------------------------------
void BaseApplication::createCamera(void){
	mCamera = mSceneMgr->createCamera( "PlayerCam" );
	mCamera->setNearClipDistance( 0.1f );
	mCamera->setAutoAspectRatio( true );

	mCameraNode = mSceneMgr->getRootSceneNode()->createChildSceneNode( "PlayerCamNode" );
	mCameraNode->setPosition( 0, 2, 4 );
	mCameraNode->attachObject( mCamera );
}
//-------------------------------------------------------------------------------------
void BaseApplication::createViewports(void){
	mViewport = mWindow->addViewport( mCamera );
	mViewport->setBackgroundColour( Ogre::ColourValue( 0, 0, 0 ) );
	mViewport->setMaterialScheme( Ogre::MSN_SHADERGEN );
}
//-------------------------------------------------------------------------------------
void BaseApplication::createFrameListener(void){
	mTrayMgr = new OgreBites::TrayManager( "InterfaceName", mWindow );
#ifdef TUMBU_DEBUG
	mTrayMgr->showFrameStats( OgreBites::TL_BOTTOMLEFT );
	mTrayMgr->toggleAdvancedFrameStats();

	Ogre::StringVector items;
	items.push_back( "cam.pX" );
	items.push_back( "cam.pY" );
	items.push_back( "cam.pZ" );
	items.push_back( "" );
	items.push_back( "cam.oW" );
	items.push_back( "cam.oX" );
	items.push_back( "cam.oY" );
	items.push_back( "cam.oZ" );
	items.push_back( "" );
	items.push_back( "Filtering" );
	items.push_back( "Poly Mode" );

	mDetailsPanel = mTrayMgr->createParamsPanel( OgreBites::TL_NONE, "DetailsPanel", 200, items );
	mDetailsPanel->setParamValue( 9, "Bilinear" );
	mDetailsPanel->setParamValue( 10, "Solid" );
	mDetailsPanel->hide();
#endif
	mTrayMgr->hideCursor();
	windowResized( mWindow );
}
//-------------------------------------------------------------------------------------
void BaseApplication::destroyScene(void){
}
//-------------------------------------------------------------------------------------
void BaseApplication::setMouseCaptured( bool captured ){
	// Automated runs (DevTest) must never grab the mouse of whoever is using the computer.
	if( DevTest::isEnabled() ){
		return;
	}
	if( !mWindows.empty() ){
		setWindowGrab( mWindows[0].native, captured );
	}
}
//-------------------------------------------------------------------------------------
bool BaseApplication::frameRenderingQueued( const Ogre::FrameEvent& evt ){
	OgreBites::ApplicationContext::frameRenderingQueued( evt );

	if( mWindow->isClosed() || mShutDown ){
		return false;
	}

	mTrayMgr->frameRendered( evt );

#ifdef TUMBU_DEBUG
	if( mDetailsPanel != NULL && mDetailsPanel->isVisible() ){
		mDetailsPanel->setParamValue( 0, Ogre::StringConverter::toString( mCamera->getDerivedPosition().x ) );
		mDetailsPanel->setParamValue( 1, Ogre::StringConverter::toString( mCamera->getDerivedPosition().y ) );
		mDetailsPanel->setParamValue( 2, Ogre::StringConverter::toString( mCamera->getDerivedPosition().z ) );
		mDetailsPanel->setParamValue( 4, Ogre::StringConverter::toString( mCamera->getDerivedOrientation().w ) );
		mDetailsPanel->setParamValue( 5, Ogre::StringConverter::toString( mCamera->getDerivedOrientation().x ) );
		mDetailsPanel->setParamValue( 6, Ogre::StringConverter::toString( mCamera->getDerivedOrientation().y ) );
		mDetailsPanel->setParamValue( 7, Ogre::StringConverter::toString( mCamera->getDerivedOrientation().z ) );
	}
#endif
	return true;
}
//-------------------------------------------------------------------------------------
void BaseApplication::windowResized( Ogre::RenderWindow* rw ){
	OgreBites::ApplicationContext::windowResized( rw );
}
//------------------------------------------------------------------------------------- input dispatch
bool BaseApplication::isListening( OgreBites::InputListener *listener ) const{
	// A handler can end the match (Quit, Game Over) and delete listeners that are still in the snapshot.
	for( std::map<std::string, OgreBites::InputListener*>::const_iterator it = mInputListeners.begin(); it != mInputListeners.end(); ++it ){
		if( it->second == listener ){
			return true;
		}
	}
	return false;
}
//-------------------------------------------------------------------------------------
std::vector<OgreBites::InputListener*> BaseApplication::listenersSnapshot(void) const{
	// Listeners may add/remove listeners while handling an event, so iterate over a copy.
	std::vector<OgreBites::InputListener*> listeners;
	for( std::map<std::string, OgreBites::InputListener*>::const_iterator it = mInputListeners.begin(); it != mInputListeners.end(); ++it ){
		listeners.push_back( it->second );
	}
	return listeners;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::keyPressed( const OgreBites::KeyboardEvent &evt ){
	if( evt.repeat ){
		return true;	// OIS did not repeat keys; the game logic expects one press per key down
	}

#ifdef TUMBU_DEBUG
	if( evt.keysym.sym == 'g' && mDetailsPanel != NULL ){ // camera details
		if( mDetailsPanel->getTrayLocation() == OgreBites::TL_NONE ){
			mTrayMgr->moveWidgetToTray( mDetailsPanel, OgreBites::TL_TOPRIGHT, 0 );
			mDetailsPanel->show();
		}else{
			mTrayMgr->removeWidgetFromTray( mDetailsPanel );
			mDetailsPanel->hide();
		}
	}else if( evt.keysym.sym == 'r' ){ // polygon mode
		Ogre::String newVal;
		Ogre::PolygonMode pm;
		switch( mCamera->getPolygonMode() ){
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
		mCamera->setPolygonMode( pm );
		if( mDetailsPanel != NULL ) mDetailsPanel->setParamValue( 10, newVal );
	}else if( evt.keysym.sym == OgreBites::SDLK_F5 ){ // reload textures
		Ogre::TextureManager::getSingleton().reloadAll();
	}else if( evt.keysym.sym == OgreBites::SDLK_PRINTSCREEN ){ // screenshot
		mWindow->writeContentsToTimestampedFile( workPath + "screenshot", ".png" );
	}
#endif

	std::vector<OgreBites::InputListener*> listeners = listenersSnapshot();
	for( size_t i = 0; i < listeners.size(); i++ ){
		if( isListening( listeners[i] ) ) listeners[i]->keyPressed( evt );
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::keyReleased( const OgreBites::KeyboardEvent &evt ){
	std::vector<OgreBites::InputListener*> listeners = listenersSnapshot();
	for( size_t i = 0; i < listeners.size(); i++ ){
		if( isListening( listeners[i] ) ) listeners[i]->keyReleased( evt );
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::mouseMoved( const OgreBites::MouseMotionEvent &evt ){
	std::vector<OgreBites::InputListener*> listeners = listenersSnapshot();
	for( size_t i = 0; i < listeners.size(); i++ ){
		if( isListening( listeners[i] ) ) listeners[i]->mouseMoved( evt );
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::mouseWheelRolled( const OgreBites::MouseWheelEvent &evt ){
	std::vector<OgreBites::InputListener*> listeners = listenersSnapshot();
	for( size_t i = 0; i < listeners.size(); i++ ){
		if( isListening( listeners[i] ) ) listeners[i]->mouseWheelRolled( evt );
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::mousePressed( const OgreBites::MouseButtonEvent &evt ){
	std::vector<OgreBites::InputListener*> listeners = listenersSnapshot();
	for( size_t i = 0; i < listeners.size(); i++ ){
		if( isListening( listeners[i] ) ) listeners[i]->mousePressed( evt );
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::mouseReleased( const OgreBites::MouseButtonEvent &evt ){
	std::vector<OgreBites::InputListener*> listeners = listenersSnapshot();
	for( size_t i = 0; i < listeners.size(); i++ ){
		if( isListening( listeners[i] ) ) listeners[i]->mouseReleased( evt );
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::textInput( const OgreBites::TextInputEvent &evt ){
	std::vector<OgreBites::InputListener*> listeners = listenersSnapshot();
	for( size_t i = 0; i < listeners.size(); i++ ){
		if( isListening( listeners[i] ) ) listeners[i]->textInput( evt );
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::axisMoved( const OgreBites::AxisEvent &evt ){
	std::vector<OgreBites::InputListener*> listeners = listenersSnapshot();
	for( size_t i = 0; i < listeners.size(); i++ ){
		if( isListening( listeners[i] ) ) listeners[i]->axisMoved( evt );
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::buttonPressed( const OgreBites::ButtonEvent &evt ){
	std::vector<OgreBites::InputListener*> listeners = listenersSnapshot();
	for( size_t i = 0; i < listeners.size(); i++ ){
		if( isListening( listeners[i] ) ) listeners[i]->buttonPressed( evt );
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool BaseApplication::buttonReleased( const OgreBites::ButtonEvent &evt ){
	std::vector<OgreBites::InputListener*> listeners = listenersSnapshot();
	for( size_t i = 0; i < listeners.size(); i++ ){
		if( isListening( listeners[i] ) ) listeners[i]->buttonReleased( evt );
	}
	return true;
}
//------------------------------------------------------------------------------------- listener registry
void BaseApplication::addInputListenerByName( OgreBites::InputListener *listener, const std::string& instanceName ){
	for( std::map<std::string, OgreBites::InputListener*>::iterator it = mInputListeners.begin(); it != mInputListeners.end(); ++it ){
		if( it->second == listener ){
			return; // already registered (keyboard, mouse and joystick share one OgreBites listener)
		}
	}
	mInputListeners[ instanceName ] = listener;
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeInputListenerByName( const std::string& instanceName ){
	std::map<std::string, OgreBites::InputListener*>::iterator it = mInputListeners.find( instanceName );
	if( it != mInputListeners.end() ){
		mInputListeners.erase( it );
	}
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeInputListenerByPointer( OgreBites::InputListener *listener ){
	std::map<std::string, OgreBites::InputListener*>::iterator it = mInputListeners.begin();
	while( it != mInputListeners.end() ){
		if( it->second == listener ){
			mInputListeners.erase( it++ );
		}else{
			++it;
		}
	}
}
//-------------------------------------------------------------------------------------
void BaseApplication::addKeyListener( OgreBites::InputListener *listener, const std::string& instanceName ){
	addInputListenerByName( listener, instanceName );
}
//-------------------------------------------------------------------------------------
void BaseApplication::addMouseListener( OgreBites::InputListener *listener, const std::string& instanceName ){
	addInputListenerByName( listener, instanceName );
}
//-------------------------------------------------------------------------------------
void BaseApplication::addJoystickListener( OgreBites::InputListener *listener, const std::string& instanceName ){
	addInputListenerByName( listener, instanceName );
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeKeyListener( const std::string& instanceName ){
	removeInputListenerByName( instanceName );
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeKeyListener( OgreBites::InputListener *listener ){
	removeInputListenerByPointer( listener );
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeMouseListener( const std::string& instanceName ){
	removeInputListenerByName( instanceName );
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeMouseListener( OgreBites::InputListener *listener ){
	removeInputListenerByPointer( listener );
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeJoystickListener( const std::string& instanceName ){
	removeInputListenerByName( instanceName );
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeJoystickListener( OgreBites::InputListener *listener ){
	removeInputListenerByPointer( listener );
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeAllListeners( void ){
	mInputListeners.clear();
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeAllKeyListeners( void ){
	mInputListeners.clear();
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeAllMouseListeners( void ){
	mInputListeners.clear();
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeAllJoystickListeners( void ){
	mInputListeners.clear();
}
//------------------------------------------------------------------------------------- collision listeners
void BaseApplication::addCollisionDetectionListener( CollisionDetectionListener *collisionListener, const std::string& instanceName ){
	if( mCollisionDetectionListeners.find( instanceName ) == mCollisionDetectionListeners.end() ){
		mCollisionDetectionListeners[ instanceName ] = collisionListener;
	}else{
		Ogre::LogManager::getSingletonPtr()->logWarning( "Collision listener not added (duplicate name): " + instanceName );
	}
}
//-------------------------------------------------------------------------------------
void BaseApplication::addCollisionDetectionListener( CollisionDetectionListener *collisionListener ){
	// The empty name is only for collision objects with a single rigid body.
	addCollisionDetectionListener( collisionListener, collisionListener->getOgreBulletRigidBody("")->getName() );
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeCollisionDetectionListener( const std::string& instanceName ){
	std::map<std::string, CollisionDetectionListener*>::iterator it = mCollisionDetectionListeners.find( instanceName );
	if( it != mCollisionDetectionListeners.end() ){
		mCollisionDetectionListeners.erase( it );
	}
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeCollisionDetectionListener( CollisionDetectionListener *collisionDetectionListener ){
	std::map<std::string, CollisionDetectionListener*>::iterator it = mCollisionDetectionListeners.begin();
	for( ; it != mCollisionDetectionListeners.end(); ++it ){
		if( it->second == collisionDetectionListener ){
			mCollisionDetectionListeners.erase( it );
			break;
		}
	}
}
//-------------------------------------------------------------------------------------
CollisionDetectionListener* BaseApplication::getCollisionListenerByName( const std::string& instanceName ){
	std::map<std::string, CollisionDetectionListener*>::iterator it = mCollisionDetectionListeners.find( instanceName );
	return ( it == mCollisionDetectionListeners.end() ) ? NULL : it->second;
}
//-------------------------------------------------------------------------------------
void BaseApplication::removeAllCollisionDetectionListeners( void ){
	mCollisionDetectionListeners.clear();
}
//-------------------------------------------------------------------------------------
