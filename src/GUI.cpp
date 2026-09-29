// Headless GUI (modernization steps B2-B5): keeps the game flow of the 2011 CEGUI interface without drawing
// anything, so the game logic can be ported and tested before the MyGUI interface exists (step B6).
//   - dialogs are queued and written to ogre.log; SPACE/ENTER answers OK, ESC answers cancel
//   - on the start screen, ENTER starts a match and ESC quits
#include "GUI.h"
#include "TUMBU.h"
//-------------------------------------------------------------------------------------
GUI* GUI::instance = NULL;
//-------------------------------------------------------------------------------------
GUI::GUI( Ogre::RenderWindow* window, Ogre::Root *root ){
	mWindow				= window;
	mRoot				= root;
	hero				= NULL;
	enemy				= NULL;
	hasActiveDialog		= false;
	waitingUserContinue	= false;
	startMenuVisible	= false;
	optionsOpen			= false;
}
//-------------------------------------------------------------------------------------
GUI::~GUI(void){
	instance = NULL;
}
//-------------------------------------------------------------------------------------
GUI* GUI::getInstance(void){
	if( instance == NULL ){
		instance = new GUI( TUMBU::getInstance()->mWindow, TUMBU::getInstance()->mRoot );
	}
	return instance;
}
//-------------------------------------------------------------------------------------
void GUI::reset(void){
	while( !dialogList.empty() ){
		dialogList.pop();
	}
	hasActiveDialog		= false;
	waitingUserContinue	= false;
	optionsOpen			= false;
	hero				= NULL;
	enemy				= NULL;
}
//-------------------------------------------------------------------------------------
void GUI::initOptions(void){
}
//-------------------------------------------------------------------------------------
bool GUI::isOptionOn(void){
	return optionsOpen;
}
//-------------------------------------------------------------------------------------
int GUI::getScreenWidth(void){
	return mWindow->getWidth();
}
//-------------------------------------------------------------------------------------
int GUI::getScreenHeight(void){
	return mWindow->getHeight();
}
//------------------------------------------------------------------------------------- dialogs
void GUI::pushDialog( const DialogEntry &dialog ){
	dialogList.push( dialog );
}
//-------------------------------------------------------------------------------------
void GUI::addAlert( const std::string &title, const std::string &message, Action okAction ){
	DialogEntry d;
	d.type = DialogEntry::ALERT;
	d.title = title;
	d.message = message;
	d.part = NULL;
	d.okAction = okAction;
	d.cancelAction = NULL;
	pushDialog( d );
}
//-------------------------------------------------------------------------------------
void GUI::addConfirm( const std::string &title, const std::string &message, Action okAction, Action cancelAction ){
	DialogEntry d;
	d.type = DialogEntry::CONFIRM;
	d.title = title;
	d.message = message;
	d.part = NULL;
	d.okAction = okAction;
	d.cancelAction = cancelAction;
	pushDialog( d );
}
//-------------------------------------------------------------------------------------
void GUI::addConversation( const std::string &characterName, const std::string &image, const std::string &message ){
	DialogEntry d;
	d.type = DialogEntry::CONVERSATION;
	d.title = characterName;
	d.message = message;
	d.image = image;
	d.part = NULL;
	d.okAction = NULL;
	d.cancelAction = NULL;
	pushDialog( d );
}
//-------------------------------------------------------------------------------------
void GUI::addShowPart( const std::string &title, const std::string &message, Part *part ){
	DialogEntry d;
	d.type = DialogEntry::SHOW_PART;
	d.title = title;
	d.message = message;
	d.part = part;
	d.okAction = NULL;
	d.cancelAction = NULL;
	pushDialog( d );
}
//-------------------------------------------------------------------------------------
void GUI::showNextDialog( bool ok ){
	if( !dialogList.empty() ){
		TUMBU::getInstance()->setGameState( TumbuEnums::IN_DIALOG );
		activeDialog = dialogList.front();
		dialogList.pop();
		hasActiveDialog = true;
		waitingUserContinue = true;
		Ogre::LogManager::getSingleton().logMessage( "[GUI] dialog: " + activeDialog.title + " - " + activeDialog.message );
	}else{
		hasActiveDialog = false;
		waitingUserContinue = false;
		TUMBU::getInstance()->setGameState( TumbuEnums::PLAYING );
	}
}
//-------------------------------------------------------------------------------------
void GUI::answerDialog( bool ok ){
	if( !hasActiveDialog ){
		return;
	}
	Action action = ok ? activeDialog.okAction : activeDialog.cancelAction;
	if( activeDialog.type != DialogEntry::CONFIRM ){
		action = activeDialog.okAction;	// alerts, conversations and part previews only have OK
	}
	if( action != NULL ){
		( this->*action )();
	}else{
		showNextDialog( ok );
	}
}
//-------------------------------------------------------------------------------------
bool GUI::isWaitingUserToContinue(void){
	return waitingUserContinue;
}
//------------------------------------------------------------------------------------- HUD
void GUI::addLog( Log::LogType logType, const std::string &message, float duration ){
	Ogre::LogManager::getSingleton().logMessage( "[GUI] log: " + message );
}
//-------------------------------------------------------------------------------------
void GUI::addSkillHit( const Ogre::Vector3 &position, const Ogre::String &message ){
}
//-------------------------------------------------------------------------------------
void GUI::startLoad( const Ogre::String &message ){
	Ogre::LogManager::getSingleton().logMessage( "[GUI] " + message );
}
//-------------------------------------------------------------------------------------
void GUI::stopLoad(void){
}
//-------------------------------------------------------------------------------------
void GUI::showBattleLayout(void){
}
//-------------------------------------------------------------------------------------
void GUI::hideBattleLayout(void){
}
//-------------------------------------------------------------------------------------
void GUI::showStartMenu(void){
	startMenuVisible = true;
	setMouseCursorVisibility( true );
}
//-------------------------------------------------------------------------------------
void GUI::hideStartMenu(void){
	startMenuVisible = false;
}
//-------------------------------------------------------------------------------------
void GUI::setHero( Character* _hero ){
	hero = _hero;
}
//-------------------------------------------------------------------------------------
void GUI::setEnemy( CharacterEnemy* _enemy ){
	enemy = _enemy;
}
//-------------------------------------------------------------------------------------
void GUI::setMouseCursorVisibility( bool visible ){
	TUMBU::getInstance()->setMouseCaptured( !visible );
}
//------------------------------------------------------------------------------------- actions
void GUI::gameStart(void){
	hideStartMenu();
	TUMBU::getInstance()->initializeDemo();
}
//-------------------------------------------------------------------------------------
void GUI::gameExit(void){
	hideStartMenu();
	TUMBU::getInstance()->shutdown();
}
//-------------------------------------------------------------------------------------
void GUI::gameOverOk(void){
	showNextDialog( true );
	TUMBU::getInstance()->finishDemo();
}
//-------------------------------------------------------------------------------------
void GUI::dialogOk(void){
	showNextDialog( true );
}
//-------------------------------------------------------------------------------------
void GUI::dialogCancel(void){
	showNextDialog( false );
}
//-------------------------------------------------------------------------------------
void GUI::startTutorialMode(void){
	TUMBU::getInstance()->getDemo()->startTutorialMode();
}
//-------------------------------------------------------------------------------------
void GUI::stopTutorialMode(void){
	TUMBU::getInstance()->getDemo()->stopTutorialMode();
	showNextDialog( true );
}
//------------------------------------------------------------------------------------- frame and input
bool GUI::frameRenderingQueued( const Ogre::FrameEvent &evt ){
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::keyPressed( const OgreBites::KeyboardEvent &evt ){
	TumbuEnums::GameState state = TUMBU::getInstance()->getGameState();
	int key = evt.keysym.sym;

	if( state == TumbuEnums::IN_DIALOG ){
		if( key == TumbuInput::KEY_SPACE || key == TumbuInput::KEY_RETURN ){
			answerDialog( true );
		}else if( key == TumbuInput::KEY_ESCAPE ){
			answerDialog( false );
		}
	}else if( state == TumbuEnums::START_SCREEN && startMenuVisible ){
		if( key == TumbuInput::KEY_RETURN ){
			gameStart();
		}else if( key == TumbuInput::KEY_ESCAPE ){
			gameExit();
		}
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::keyReleased( const OgreBites::KeyboardEvent &evt ){
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::mouseMoved( const OgreBites::MouseMotionEvent &evt ){
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::mousePressed( const OgreBites::MouseButtonEvent &evt ){
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::mouseReleased( const OgreBites::MouseButtonEvent &evt ){
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::buttonPressed( const OgreBites::ButtonEvent &evt ){
	TumbuEnums::GameState state = TUMBU::getInstance()->getGameState();
	if( state == TumbuEnums::IN_DIALOG ){
		if( evt.button == TumbuInput::PAD_OK ){
			answerDialog( true );
		}else if( evt.button == TumbuInput::PAD_CANCEL ){
			answerDialog( false );
		}
	}else if( state == TumbuEnums::START_SCREEN && startMenuVisible && evt.button == TumbuInput::PAD_MENU ){
		gameStart();
	}
	return true;
}
//-------------------------------------------------------------------------------------
