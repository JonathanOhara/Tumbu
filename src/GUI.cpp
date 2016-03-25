#include "GUI.h"
#include "TUMBU.h"
#include "Clock.h"
GUI* GUI::instance = NULL;
//-------------------------------------------------------------------------------------
GUI::GUI(Ogre::RenderWindow* window, Ogre::Root *root){
	mWindow = window;
	mRoot = root;

	Ogre::String logPath( TUMBU::getInstance()->workPath + "cegui.log");

	defaultLogger = new CEGUI::DefaultLogger();

	CEGUI::Logger::getSingleton().setLogFilename(TUMBU::getInstance()->workPath + "\\cegui.log", false);
	cegui = &CEGUI::OgreRenderer::bootstrapSystem();


	CEGUI::Imageset::setDefaultResourceGroup("imagesets");
	CEGUI::Font::setDefaultResourceGroup("Fonts");
	CEGUI::Scheme::setDefaultResourceGroup("Schemes");
	CEGUI::WidgetLookManager::setDefaultResourceGroup("LookNFeel");
	CEGUI::WindowManager::setDefaultResourceGroup("Layouts");

	CEGUI::SchemeManager::getSingleton().create("TaharezLook.scheme");
	CEGUI::System::getSingleton().setDefaultMouseCursor("TaharezLook", "MouseArrow");

	CEGUI::ImagesetManager::getSingleton().create("RobotFaces.imageset", "imagesets");

	waitingUserContinue = false;
	auxSceneMgrActive= false;

	lastConversationTop = false;
	lastConversationName = "";

	lastActiveDialog = NULL;

	CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();
	rootWindow = wmgr.createWindow("DefaultWindow", "root");
	CEGUI::System::getSingleton().setGUISheet( rootWindow );
	
	rootWindow->setVisible(true);

	loadStartScreen();
	loadLoadingLayout();

	logManager = new LogManager(this);
	skillHitManager = new SkillHitManager(this);

	lastDialogConversation = false;

	timeSinceLastFrame = 0;

	mouseRunning = false;
	mouseRunningX = 0;
	mouseRunningY = 0;

	hero = NULL;
	enemy = NULL;
}
//-------------------------------------------------------------------------------------
GUI::~GUI(void){
	delete logManager;
	delete skillHitManager;

	unbuildCharacterPreview();

	rootWindow->destroy();

	CEGUI::OgreRenderer::destroySystem();
	delete defaultLogger;

	instance = NULL;
}
//-------------------------------------------------------------------------------------
GUI* GUI::getInstance(){
	if( instance == NULL){
		instance = new GUI(TUMBU::getInstance()->mWindow, TUMBU::getInstance()->mRoot);
	}
	return instance;
}
//-------------------------------------------------------------------------------------
void GUI::reset(){
	logManager->reset();

	waitingUserContinue = false;
	auxSceneMgrActive= false;

	hero = NULL;
	enemy = NULL;

	lastConversationTop = false;
	lastConversationName = "";

	lastDialogConversation = false;

	lastActiveDialog = NULL;
	
	hideBattleLayout();
	hideAllOptions();

	skillHitManager->reset();

	rootWindow->setVisible(true);

	CEGUI::MouseCursor::getSingleton().show();
}
//-------------------------------------------------------------------------------------
bool GUI::frameRenderingQueued(const Ogre::FrameEvent &evt){
	timeSinceLastFrame = evt.timeSinceLastFrame;

	if( isOptionOn() ) {
		if( auxSceneMgrActive ){
			auxRobotNode->yaw( Ogre::Radian(0.5f * timeSinceLastFrame) );
		}
	}else if ( isWaitingUserToContinue() ){
		if ( lastActiveDialog != NULL ){
			lastActiveDialog->update( timeSinceLastFrame );
		}
	}else if( TUMBU::getInstance()->isPlaying() ){
		updateLeftTopHpBar(hero->hp, hero->hpPercent);
		updateLeftTopApBar(hero->ap, hero->apPercent);

		updateRightBottomHpBar(enemy->hp, enemy->hpPercent);
		updateRightBottomApBar(enemy->ap, enemy->apPercent);

		logManager->update( timeSinceLastFrame );
		skillHitManager->update( timeSinceLastFrame );
	}

	if( mouseRunning ){
		CEGUI::System::getSingleton().injectMouseMove(mouseRunningX * timeSinceLastFrame, mouseRunningY * timeSinceLastFrame);
	}

	return true;
}
//-------------------------------------------------------------------------------------
void GUI::initOptions(){
	loadBattleLayout();
	loadConversationLayout();
	loadDialogLayout();
	loadMenuLayout();
}
//-------------------------------------------------------------------------------------
void GUI::loadLoadingLayout(){
	CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();
	loadingLayout = CEGUI::WindowManager::getSingleton().loadWindowLayout( "loading.layout" );

	mainLoading				= wmgr.getWindow("RootLoading/Loading");
	loadingText				= (CEGUI::Window*)mainLoading->getChild("RootLoading/Loading/LoadingText");

	mainLoading->setVisible(false);
	mainLoading->setAlwaysOnTop(false);

	rootWindow->addChildWindow(mainLoading);
}
//-------------------------------------------------------------------------------------
void GUI::loadStartScreen(){
	CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();
	startScreenLayout = CEGUI::WindowManager::getSingleton().loadWindowLayout( "startScreen.layout" );

	startScreenMenu					= wmgr.getWindow("RootStartScreen/StartMenu");
	startScreenOptionMenu			= wmgr.getWindow("RootStartScreen/OptionsMenu");

	startScreenCampaignButton		= (CEGUI::Window*)startScreenMenu->getChild("RootStartScreen/StartMenu/campaignButton");
	startScreenOptionsButton		= (CEGUI::Window*)startScreenMenu->getChild("RootStartScreen/StartMenu/optionsButton");
	startScreenExitButton			= (CEGUI::Window*)startScreenMenu->getChild("RootStartScreen/StartMenu/exitButton");

	startScreenOptionSkyCombo		= (CEGUI::Combobox*)startScreenOptionMenu->getChild("RootStartScreen/OptionsMenu/skyCombo");
	startScreenOptionShadowsCombo	= (CEGUI::Combobox*)startScreenOptionMenu->getChild("RootStartScreen/OptionsMenu/shadowsCombo");
	startScreenOptionOkButton		= (CEGUI::Window*)startScreenOptionMenu->getChild("RootStartScreen/OptionsMenu/okButton");
	startScreenOptionDefaultButton	= (CEGUI::Window*)startScreenOptionMenu->getChild("RootStartScreen/OptionsMenu/defaultButton");
	startScreenOptionCancelButton	= (CEGUI::Window*)startScreenOptionMenu->getChild("RootStartScreen/OptionsMenu/cancelButton");

	startScreenOptionSkyCombo->addItem( new CEGUI::ListboxTextItem("Low Quality",0) );
	startScreenOptionSkyCombo->addItem( new CEGUI::ListboxTextItem("High Quality",1) );

	startScreenOptionShadowsCombo->addItem( new CEGUI::ListboxTextItem("No Shadows",0) );
	startScreenOptionShadowsCombo->addItem( new CEGUI::ListboxTextItem("Medium Quality",1) );

	startScreenMenu->setVisible(false);
	startScreenOptionMenu->setVisible(false);

	startScreenCampaignButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::gameStart, this));
	startScreenOptionsButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::gameOptions, this));
	startScreenExitButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::gameExit, this));
		
	startScreenOptionOkButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::gameOptionsOk, this));
	startScreenOptionDefaultButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::gameOptionsDefault, this));
	startScreenOptionCancelButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::gameOptionsCancel, this));

	rootWindow->addChildWindow(startScreenMenu);
	rootWindow->addChildWindow(startScreenOptionMenu);
}
//-------------------------------------------------------------------------------------
void GUI::loadBattleLayout(){
	CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();
	battleLayout =  CEGUI::WindowManager::getSingleton().loadWindowLayout( "battle.layout" );
	
	leftTop					= wmgr.getWindow("RootBattle/leftTop");
	mainHpProgressBar		= (CEGUI::ProgressBar*)leftTop->getChild("RootBattle/leftTop/hp_progress_bar");
	mainApProgressBar		= (CEGUI::ProgressBar*)leftTop->getChild("RootBattle/leftTop/ap_progress_bar");
	leftTopHpValue			= leftTop->getChild("RootBattle/leftTop/value_hp");
	leftTopApValue			= leftTop->getChild("RootBattle/leftTop/value_ap");

	//RootBattle/rightBottom/value_hp
	rightBottom				= wmgr.getWindow("RootBattle/rightBottom");
	enemyHpProgressBar		= (CEGUI::ProgressBar*)rightBottom->getChild("RootBattle/rightBottom/hp_progress_bar");
	enemyApProgressBar		= (CEGUI::ProgressBar*)rightBottom->getChild("RootBattle/rightBottom/ap_progress_bar");
	rightBottomHpValue		= rightBottom->getChild("RootBattle/rightBottom/value_hp");
	rightBottomApValue		= rightBottom->getChild("RootBattle/rightBottom/value_ap");

	rootLog					= wmgr.getWindow("RootBattle/log");

	log1					= rootLog->getChild("RootBattle/log/log1");
	log2					= rootLog->getChild("RootBattle/log/log2");
	log3					= rootLog->getChild("RootBattle/log/log3");
	log4					= rootLog->getChild("RootBattle/log/log4");

	rightBottomHpValue->setText( "???" );
	rightBottomApValue->setText( "???" );

	rootLog->setVisible(false);
	log1->setVisible( false );
	log2->setVisible( false );
	log3->setVisible( false );
	log4->setVisible( false );

	leftTop->setAlwaysOnTop(false);
	leftTop->setVisible(false);
	mainHpProgressBar->setVisible(false);
	mainApProgressBar->setVisible(false);

	rightBottom->setAlwaysOnTop(false);
	rightBottom->setVisible(false);
	enemyHpProgressBar->setVisible(false);
	enemyApProgressBar->setVisible(false);
	
	rootWindow->addChildWindow(leftTop);
	rootWindow->addChildWindow(rightBottom);
	rootWindow->addChildWindow(rootLog);
}
//-------------------------------------------------------------------------------------
void GUI::loadConversationLayout(){
	CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();
	conversationLayout = CEGUI::WindowManager::getSingleton().loadWindowLayout( "conversation.layout" );

	mainConversation					= wmgr.getWindow("RootConversation/Main");

	conversationTop						= (CEGUI::Window*)mainConversation->getChild("RootConversation/Main/conversationTop");
	conversationBottom					= (CEGUI::Window*)mainConversation->getChild("RootConversation/Main/conversationBottom");

	conversationTopImage				= (CEGUI::Window*)conversationTop->getChild("RootConversation/Main/conversationTop/image");
	conversationTopMessage				= (CEGUI::Window*)conversationTop->getChild("RootConversation/Main/conversationTop/text");
	conversationTopContinueButton		= (CEGUI::Window*)conversationTop->getChild("RootConversation/Main/conversationTop/moreButton");

	conversationBottomImage				= (CEGUI::Window*)conversationBottom->getChild("RootConversation/Main/conversationBottom/image");
	conversationBottomMessage			= (CEGUI::Window*)conversationBottom->getChild("RootConversation/Main/conversationBottom/text");
	conversationBottomContinueButton	= (CEGUI::Window*)conversationBottom->getChild("RootConversation/Main/conversationBottom/moreButton");

	conversationTopContinueButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::continueTopConversation, this));
	conversationBottomContinueButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::continueBottomConversation, this));

	mainConversation->setVisible(false);
	mainConversation->setAlwaysOnTop(false);

	conversationTop->setVisible(false);
	conversationTop->setAlwaysOnTop(false);

	conversationBottom->setVisible(false);
	conversationBottom->setAlwaysOnTop(false);

	rootWindow->addChildWindow(mainConversation);
}
//-------------------------------------------------------------------------------------
void GUI::loadDialogLayout(){
	CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();
	conversationLayout = CEGUI::WindowManager::getSingleton().loadWindowLayout( "dialogSystem.layout" );


	mainDialog							= wmgr.getWindow("RootDialog");
	mainDialogAlert						= wmgr.getWindow("RootDialog/alert");
	mainDialogConfirm					= wmgr.getWindow("RootDialog/confirm");
	mainDialogShowPart					= wmgr.getWindow("RootDialog/showPart");

	dialogShowPartImage					= (CEGUI::Window*)mainDialog->getChild("RootDialog/staticImage");

	dialogAlertText						= (CEGUI::Window*)mainDialogAlert->getChild("RootDialog/alert/text");
	dialogAlertOkButton					= (CEGUI::Window*)mainDialogAlert->getChild("RootDialog/alert/okbutton");

	dialogConfirmText					= (CEGUI::Window*)mainDialogConfirm->getChild("RootDialog/confirm/text");
	dialogConfirmCancelButton			= (CEGUI::Window*)mainDialogConfirm->getChild("RootDialog/confirm/cancelButton");
	dialogConfirmOkButton				= (CEGUI::Window*)mainDialogConfirm->getChild("RootDialog/confirm/okbutton");

	dialogShowPartText					= (CEGUI::Window*)mainDialogShowPart->getChild("RootDialog/showPart/text");
	dialogShowPartOkButton				= (CEGUI::Window*)mainDialogShowPart->getChild("RootDialog/showPart/okbutton");

	dialogAlertOkButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::dialogOk, this));
	dialogConfirmOkButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::dialogOk, this));
	dialogShowPartOkButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::dialogShowPartOk, this));

	dialogConfirmCancelButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::dialogCancel, this));

	mainDialog->setVisible(false);
	mainDialog->setAlwaysOnTop(false);

	mainDialogAlert->setVisible(false);
	mainDialogAlert->setAlwaysOnTop(false);

	mainDialogConfirm->setVisible(false);
	mainDialogConfirm->setAlwaysOnTop(false);

	mainDialogShowPart->setVisible(false);
	mainDialogShowPart->setAlwaysOnTop(false);

	rootWindow->addChildWindow(mainDialog);
}
//-------------------------------------------------------------------------------------
void GUI::loadMenuLayout(){
	CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();
	menuLayout = CEGUI::WindowManager::getSingleton().loadWindowLayout( "menu.layout" );

	options					= wmgr.getWindow("RootMenu/options");

	optionsStatusButton		= options->getChild("RootMenu/options/status");
	optionsInventoryButton	= options->getChild("RootMenu/options/inventory");
	optionsSkillsButton		= options->getChild("RootMenu/options/skills");
	optionsHelpButton		= options->getChild("RootMenu/options/help");
	optionsQuitButton		= options->getChild("RootMenu/options/quit");
	clock					= options->getChild("RootMenu/options/clock");

	optionsStatusWindow		= options->getChild("RootMenu/options/status_window");
	optionsInventoryWindow	= options->getChild("RootMenu/options/inventory_window");
	optionsSkillsWindow		= options->getChild("RootMenu/options/skills_window");
	optionsHelpWindow		= options->getChild("RootMenu/options/help_window");

	optionStatusHp			= optionsStatusWindow->getChild("RootMenu/options/status_window/hp");
	optionStatusAp			= optionsStatusWindow->getChild("RootMenu/options/status_window/ap");
	optionStatusAttack		= optionsStatusWindow->getChild("RootMenu/options/status_window/attack");
	optionStatusDeffense	= optionsStatusWindow->getChild("RootMenu/options/status_window/deffense");
	optionStatusVelocity	= optionsStatusWindow->getChild("RootMenu/options/status_window/velocity");
	
	optionSkillsMovesListLabel	= optionsSkillsWindow->getChild("RootMenu/options/skills_window/movesListLabel");
	optionSkillsStatesValue		= optionsSkillsWindow->getChild("RootMenu/options/skills_window/details")->getChild("RootMenu/options/skills_window/details/statsValue");

	optionInventoryHeadCombo		= (CEGUI::Combobox*)optionsInventoryWindow->getChild("RootMenu/options/inventory_window/headCombo");
	optionInventoryBodyCombo		= (CEGUI::Combobox*)optionsInventoryWindow->getChild("RootMenu/options/inventory_window/bodyCombo");
	optionInventoryRightArmCombo	= (CEGUI::Combobox*)optionsInventoryWindow->getChild("RootMenu/options/inventory_window/rightArmCombo");
	optionInventoryLeftArmCombo		= (CEGUI::Combobox*)optionsInventoryWindow->getChild("RootMenu/options/inventory_window/leftArmCombo");
	optionInventoryLegsCombo		= (CEGUI::Combobox*)optionsInventoryWindow->getChild("RootMenu/options/inventory_window/legsCombo");
	optionInventoryRenderToTexture  = optionsInventoryWindow->getChild("RootMenu/options/inventory_window/background_inventory");

	clock->setText("06:00");

	options->setVisible(false);
	optionsStatusWindow->setVisible(false);
	optionsInventoryWindow->setVisible(false);
	optionsSkillsWindow->setVisible(false);
	optionsHelpWindow->setVisible(false);	

	optionsStatusButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::isOptionStatusOn, this));
	optionsInventoryButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::isOptionInventoryOn, this));
	optionsSkillsButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::isOptionSkillsOn, this));
	optionsHelpButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::isOptionHelpOn, this));
	optionsQuitButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::quit, this));

	optionInventoryHeadCombo->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&GUI::changeRobotHead, this));
	optionInventoryBodyCombo->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&GUI::changeRobotBody, this));
	optionInventoryRightArmCombo->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&GUI::changeRobotRightArm, this));
	optionInventoryLeftArmCombo->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&GUI::changeRobotLeftArm, this));
	optionInventoryLegsCombo->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&GUI::changeRobotLegs, this));

	options->setVisible(false);

	rootWindow->addChildWindow(options);
}
//-------------------------------------------------------------------------------------
void GUI::turnOptionOnOff(){
	if ( !isWaitingUserToContinue() ){
		if( options->isVisible() ){
			CEGUI::MouseCursor::getSingleton().hide( );
			options->setVisible(false);
		
			unbuildCharacterPreview();

			if( dialogList.size() == 0 ){
				TUMBU::getInstance()->setGameState( TumbuEnums::PLAYING );
			}
		}else {
			CEGUI::MouseCursor::getSingleton().show( );
			options->setVisible(true);
			hideAllOptions();

			updateClockTime();
			TUMBU::getInstance()->setGameState( TumbuEnums::PAUSED );
		}
	}
}
//------------------------------------------------------------------------------------
void GUI::unbuildCharacterPreview(){
	if( auxSceneMgrActive ){			
		headPart->unbuild();
		bodyPart->unbuild();
		rightArmPart->unbuild();
		leftArmPart->unbuild();
		legsPart->unbuild();

		delete headPart;
		delete bodyPart;
		delete rightArmPart;
		delete leftArmPart;
		delete legsPart;

		auxSceneMgr->destroyCamera(auxCamera);
			
		renderTexture->removeAllViewports();
		renderTexture->removeAllListeners();
		mRoot->getTextureManager()->remove("RTT");
			
		mRoot->destroySceneManager(auxSceneMgr);

		CEGUI::ImagesetManager::getSingleton().destroy( CEGUI::ImagesetManager::getSingleton().get("RTTImageset") );
		auxSceneMgrActive = false;
	}
}
//-------------------------------------------------------------------------------------
bool GUI::isOptionOn(){
	return options->isVisible();
}
//-------------------------------------------------------------------------------------
bool GUI::isWaitingUserToContinue(){
	return waitingUserContinue;
}
//-------------------------------------------------------------------------------------
void GUI::startLoad(Ogre::String message){
	mainLoading->setVisible(true);
	loadingText->setText(message);
}
//-------------------------------------------------------------------------------------
void GUI::stopLoad(){
	mainLoading->setVisible(false);
}
//-------------------------------------------------------------------------------------
void GUI::showBattleLayout(){
	leftTop->setVisible(true);
	mainHpProgressBar->setVisible(true);
	mainApProgressBar->setVisible(true);

	rightBottom->setVisible(true);
	enemyHpProgressBar->setVisible(true);
	enemyApProgressBar->setVisible(true);
}
//-------------------------------------------------------------------------------------
void GUI::hideBattleLayout(){
	leftTop->setVisible(false);
	mainHpProgressBar->setVisible(false);
	mainApProgressBar->setVisible(false);

	rightBottom->setVisible(false);
	enemyHpProgressBar->setVisible(false);
	enemyApProgressBar->setVisible(false);
}
//-------------------------------------------------------------------------------------
void GUI::setMouseCursorVisibility( bool visibility ){
	if( visibility ){
		CEGUI::MouseCursor::getSingleton().show();
	}else{
		CEGUI::MouseCursor::getSingleton().hide();
	}
}
//-------------------------------------------------------------------------------------
bool GUI::mouseMoved(const OIS::MouseEvent &arg){
	if( !TUMBU::getInstance()->isPlaying() ){
		OIS::MouseState state = arg.state;
		CEGUI::System::getSingleton().injectMouseMove(state.X.rel, state.Y.rel);
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id){
	CEGUI::System::getSingleton().injectMouseButtonDown(convertOisCegui(id));
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id){
	CEGUI::System::getSingleton().injectMouseButtonUp(convertOisCegui(id));
	return true;
}
//-------------------------------------------------------------------------------------
CEGUI::MouseButton GUI::convertOisCegui(const OIS::MouseButtonID id){
    switch (id){
    case OIS::MB_Left:
		return CEGUI::LeftButton;
    case OIS::MB_Right:
        return CEGUI::RightButton;
    case OIS::MB_Middle:
        return CEGUI::MiddleButton;
    default:
        return CEGUI::LeftButton;
    }
}
//-------------------------------------------------------------------------------------
bool GUI::keyPressed(const OIS::KeyEvent &arg){
	CEGUI::System &sys = CEGUI::System::getSingleton();
	sys.injectKeyDown(arg.key);
    sys.injectChar(arg.text);

	switch (arg.key){
	case OIS::KC_SPACE:
		if( TUMBU::getInstance()->getGameState() == TumbuEnums::IN_DIALOG ){
			showNextDialogByKeyBoard( true );
		}
		break;
	case OIS::KC_ESCAPE:
		if( TUMBU::getInstance()->getGameState() == TumbuEnums::IN_DIALOG ){
			showNextDialogByKeyBoard( false );
		}else if( TUMBU::getInstance()->getGameState() != TumbuEnums::START_SCREEN ){
			turnOptionOnOff( );
		}
		break;
	}

	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::keyReleased(const OIS::KeyEvent &arg){
	CEGUI::System::getSingleton().injectKeyUp(arg.key);
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::povMoved( const OIS::JoyStickEvent &e, int pov ) {
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::axisMoved( const OIS::JoyStickEvent &e, int axis ) {
	int abs = e.state.mAxes[axis].abs;

	if( !TUMBU::getInstance()->isPlaying() ){
		switch( axis ){
		case 2:
			mouseRunningY = abs / 66;
			break;
		case 3:
			mouseRunningX = abs / 66;		
			break;
		}	

		if( mouseRunningX == 0 && mouseRunningY == 0 ){
			mouseRunning = false;
		}else{
			mouseRunning = true;
		}
	}

	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::sliderMoved( const OIS::JoyStickEvent &e, int sliderID ) {
	return true;
}
//------------------------------------------------------------------------------------- 
bool GUI::buttonPressed( const OIS::JoyStickEvent &e, int button ) {
	switch( button ){
	case 2:
		if( !TUMBU::getInstance()->isPlaying() ){
			CEGUI::System::getSingleton().injectMouseButtonDown(CEGUI::LeftButton);
		}
		break;
	case 9:
		if( TUMBU::getInstance()->getGameState() == TumbuEnums::IN_DIALOG ){
			showNextDialogByKeyBoard( true );
		}else if( TUMBU::getInstance()->getGameState() != TumbuEnums::START_SCREEN ){
			turnOptionOnOff( );
		}
		break;
	case 0:
		if( TUMBU::getInstance()->getGameState() == TumbuEnums::IN_DIALOG ){
			showNextDialogByKeyBoard( false );
		}
		break;
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::buttonReleased( const OIS::JoyStickEvent &e, int button ) {
	switch( button ){
	case 2:
		if( !TUMBU::getInstance()->isPlaying() ){
			CEGUI::System::getSingleton().injectMouseButtonUp(CEGUI::LeftButton);
		}

	}
	return true;
}
//-------------------------------------------------------------------------------------
int GUI::getScreenWidth(){
	return mWindow->getWidth();
}
//-------------------------------------------------------------------------------------
int GUI::getScreenHeight(){
	return mWindow->getHeight();
}
//-------------------------------------------------------------------------------------
void GUI::updateLeftTopHpBar(float value, float percent){
	leftTopHpValue->setText( Ogre::StringConverter::toString((int)value).c_str() );
	mainHpProgressBar->setProgress(percent);
}
//-------------------------------------------------------------------------------------
void GUI::updateLeftTopApBar(float value, float percent){
	leftTopApValue->setText( Ogre::StringConverter::toString((int) value).c_str() );
	mainApProgressBar->setProgress(percent);
}
//-------------------------------------------------------------------------------------
void GUI::updateRightBottomHpBar(float value, float percent){
	enemyHpProgressBar->setProgress( percent );
}
//-------------------------------------------------------------------------------------
void GUI::updateRightBottomApBar(float value, float percent){
	enemyApProgressBar->setProgress( percent );
}
//-------------------------------------------------------------------------------------
void GUI::updateClockTime(){
	clock->setText( Clock::getInstance()->getClockFormated() );
}
//-------------------------------------------------------------------------------------
Alert* GUI::addAlert( std::string title, std::string message ){
	Alert* alert = new Alert( title, message );
	alert->setGUI( mainDialogAlert, dialogAlertText, dialogAlertOkButton);
	dialogList.push( alert );

	dialogAlertOkButton->removeAllEvents();
	dialogAlertOkButton->subscribeEvent( CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber( &GUI::dialogOk, this ) );

	return alert;
}
//-------------------------------------------------------------------------------------
Alert* GUI::addAlert( std::string title, std::string message, bool (GUI::*function)(const CEGUI::EventArgs&) ){
	Alert* alert = new Alert( title, message );
	alert->setGUI( mainDialogAlert, dialogAlertText, dialogAlertOkButton);
	dialogList.push( alert );

	dialogAlertOkButton->removeAllEvents();
	dialogAlertOkButton->subscribeEvent( CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber( function, this ) );

	return alert;
}
//-------------------------------------------------------------------------------------
Confirm* GUI::addConfirm( std::string title, std::string message ){
	Confirm* confirm = new Confirm( title, message );
	confirm->setGUI( mainDialogConfirm, dialogConfirmText, dialogConfirmOkButton, dialogConfirmCancelButton);
	dialogList.push( confirm );

	dialogConfirmOkButton->removeAllEvents();
	dialogConfirmOkButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::dialogOk, this));

	dialogConfirmCancelButton->removeAllEvents();
	dialogConfirmCancelButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::dialogCancel, this));

	return confirm;
}
//-------------------------------------------------------------------------------------
Confirm* GUI::addConfirm( std::string title, std::string message, bool (GUI::*okFunction)(const CEGUI::EventArgs&), bool (GUI::*cancelFunction)(const CEGUI::EventArgs&) ){
	Confirm* confirm = new Confirm( title, message );
	confirm->setGUI( mainDialogConfirm, dialogConfirmText, dialogConfirmOkButton, dialogConfirmCancelButton);
	dialogList.push( confirm );

	dialogConfirmOkButton->removeAllEvents();
	dialogConfirmOkButton->subscribeEvent( CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber( okFunction, this ) );

	dialogConfirmCancelButton->removeAllEvents();
	dialogConfirmCancelButton->subscribeEvent( CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber( cancelFunction, this ) );

	return confirm;
}
//-------------------------------------------------------------------------------------
Conversation* GUI::addConversation( std::string characterName, std::string image, std::string message ){
	Conversation* conversation = new Conversation(characterName, image, message);
	conversation->setGUI( mainConversation, conversationTop, conversationTopImage, conversationTopMessage, conversationTopContinueButton, conversationBottom, conversationBottomImage, conversationBottomMessage, conversationBottomContinueButton);
	dialogList.push( conversation );

	return conversation;
}
//-------------------------------------------------------------------------------------
ShowPart* GUI::addShowPart( std::string title, std::string message, Part *part ){
	ShowPart* showPart = new ShowPart( title, message, part, mRoot, cegui );
	showPart->setGUI( mainDialogShowPart, dialogShowPartText, dialogShowPartImage, dialogShowPartOkButton );
	dialogList.push( showPart );

	return showPart;
}
//-------------------------------------------------------------------------------------
void GUI::showNextDialogByKeyBoard(bool ok){
	if ( lastActiveDialog  != NULL ){
		if( lastActiveDialog->getType() == Dialog::CONVERSATION ) {
			showNextDialog( ok );
		}else{
			std::string type;

			switch( lastActiveDialog->getType() ){
			case Dialog::ALERT:
				type = "alert";
				break;
			case Dialog::CONFIRM:
				type = "confirm";
				break;
			case Dialog::SHOW_PART:
				type = "showPart";
				break;
			}

			std::string childName(mainDialog->getName().c_str());
			childName += "/" + type;
			CEGUI::Window *child = mainDialog->getChild( childName );
			if( ok ){
				childName += "/okbutton";
			}else{
				if( lastActiveDialog->getType() == Dialog::CONFIRM){
					childName += "/cancelButton";
				}else{
					childName += "/okbutton";
				}
			}
			CEGUI::Window *okButton = child->getChild( childName );
			
			CEGUI::EventArgs eventArgs;
			okButton->fireEvent(CEGUI::PushButton::EventClicked, eventArgs); 
		}
	}
}
//-------------------------------------------------------------------------------------
void GUI::showNextDialog(bool ok){
	if( dialogList.size() > 0 ){
		CEGUI::MouseCursor::getSingleton().show();

		TUMBU::getInstance()->setGameState( TumbuEnums::IN_DIALOG );
		if ( lastActiveDialog  != NULL ){
			delete lastActiveDialog;
		}
		waitingUserContinue = true;
		mainConversation->setVisible(true);
		lastActiveDialog = dialogList.front();
		dialogList.pop();
		
		mainDialog->setAlwaysOnTop(false);
		mainDialogAlert->setAlwaysOnTop(false);
		mainDialogConfirm->setAlwaysOnTop(false);
		mainDialogShowPart->setAlwaysOnTop(false);

		dialogShowPartImage->setVisible(false);
		mainConversation->setVisible(false);
		mainDialog->setVisible(false);
		mainDialogAlert->setVisible(false);
		mainDialogConfirm->setVisible(false);
		mainDialogShowPart->setVisible(false);

		if( lastActiveDialog->getType() == Dialog::CONVERSATION ) {
			Conversation* conversation = (Conversation*) lastActiveDialog;
			if( lastConversationName != conversation->mCharacterName ){
				lastConversationTop = !lastConversationTop;
				lastConversationName = conversation->mCharacterName;		
			}
			conversation->showConversationOnTop = lastConversationTop;

			lastDialogConversation = true;
		}else{
			mainDialog->setVisible(true);
			mainDialog->setAlwaysOnTop(true);

			lastDialogConversation = false;
		}

		lastActiveDialog->showDialog();

	}else{
		if ( lastActiveDialog  != NULL ){
			delete lastActiveDialog;
		}

		waitingUserContinue = false;
		lastConversationTop = false;
		lastConversationName = "";	

		mainConversation->setVisible(false);
		mainDialog->setVisible(false);

		mainDialogAlert->setVisible(false);
		mainDialogConfirm->setVisible(false);
		mainDialogShowPart->setVisible(false);

		conversationTop->setVisible(false);
		conversationBottom->setVisible(false);

		lastActiveDialog = NULL;

		CEGUI::MouseCursor::getSingleton().hide();

		TUMBU::getInstance()->setGameState( TumbuEnums::PLAYING );
	}
}
//-------------------------------------------------------------------------------------
void GUI::addLog( Log::LogType _logType, std::string message, float duration ){
	logManager->addLog( _logType, message, duration );
}
//-------------------------------------------------------------------------------------
void GUI::addSkillHit( Ogre::Vector3 position, Ogre::String message ){
	skillHitManager->addSkillHit( position, message );
}
//-------------------------------------------------------------------------------------
void GUI::showStartMenu(){
	startScreenMenu->setVisible(true);
	startScreenMenu->setAlwaysOnTop(true);
}
//-------------------------------------------------------------------------------------
void GUI::hideStartMenu(){
	startScreenMenu->setVisible(false);
	startScreenMenu->setAlwaysOnTop(false);
}
//-------------------------------------------------------------------------------------
void GUI::showStartOptionsMenu(){
	startScreenOptionMenu->setVisible(true);
	startScreenOptionMenu->setAlwaysOnTop(true);
	loadStartOptionsMenu();
}
//-------------------------------------------------------------------------------------
void GUI::hideStartOptionsMenu(){
	startScreenOptionMenu->setVisible(false);
	startScreenOptionMenu->setAlwaysOnTop(false);
}
//-------------------------------------------------------------------------------------
void GUI::saveStartOptionsMenu(){
	TUMBU::getInstance()->setSkyQuality( startScreenOptionSkyCombo->getSelectedItem()->getID() );

	cout << "ID = " << startScreenOptionShadowsCombo->getSelectedItem()->getID() << endl;

	switch( startScreenOptionShadowsCombo->getSelectedItem()->getID() ){
	case 0:	
		TUMBU::getInstance()->setCastShadows( false );
		TUMBU::getInstance()->setShadowTechnique( Ogre::SHADOWTYPE_NONE );
		TUMBU::getInstance()->setShadowColor( Ogre::ColourValue(0.4,0.4,0.4) );
		TUMBU::getInstance()->setShadowFarDistance( 100 );
		TUMBU::getInstance()->setShadowTextureSize( 256 );
		TUMBU::getInstance()->setShadowTextureCount( 1 );
		break;
	case 1:
		TUMBU::getInstance()->setCastShadows( true );
		TUMBU::getInstance()->setShadowTechnique( Ogre::SHADOWTYPE_TEXTURE_MODULATIVE );
		TUMBU::getInstance()->setShadowColor( Ogre::ColourValue(0.4,0.4,0.4) );
		TUMBU::getInstance()->setShadowFarDistance( 1000 );
		TUMBU::getInstance()->setShadowTextureSize( 1024 );
		TUMBU::getInstance()->setShadowTextureCount( 1 );
		break;
	case 2:
		TUMBU::getInstance()->setCastShadows( true );
		TUMBU::getInstance()->setShadowTechnique( Ogre::SHADOWTYPE_TEXTURE_ADDITIVE );
		TUMBU::getInstance()->setShadowColor( Ogre::ColourValue(0.4,0.4,0.4) );
		TUMBU::getInstance()->setShadowFarDistance( 500 );
		TUMBU::getInstance()->setShadowTextureSize( 512 );
		TUMBU::getInstance()->setShadowTextureCount( 1 );
		break;
	}
}
//-------------------------------------------------------------------------------------
void GUI::loadStartOptionsMenu(){
	startScreenOptionSkyCombo->setItemSelectState( 
		startScreenOptionSkyCombo->getListboxItemFromIndex( TUMBU::getInstance()->getSkyQuality() ), true);

	switch( TUMBU::getInstance()->getShadowTechnique() ){
	case Ogre::SHADOWTYPE_NONE:
		cout << "0" << endl;
		startScreenOptionShadowsCombo->setItemSelectState(startScreenOptionShadowsCombo->getListboxItemFromIndex( 0 ), true);
		break;
	case Ogre::SHADOWTYPE_TEXTURE_MODULATIVE:
		cout << "0" << endl;
		startScreenOptionShadowsCombo->setItemSelectState(startScreenOptionShadowsCombo->getListboxItemFromIndex( 1 ), true);
		break;
	case Ogre::SHADOWTYPE_TEXTURE_ADDITIVE:
		cout << "0" << endl;
		startScreenOptionShadowsCombo->setItemSelectState(startScreenOptionShadowsCombo->getListboxItemFromIndex( 2 ), true);		
		break;
	default:
		cout << "0" << endl;
		startScreenOptionShadowsCombo->setItemSelectState(startScreenOptionShadowsCombo->getListboxItemFromIndex( 0 ), true);
		break;
	}
}
//-------------------------------------------------------------------------------------
void GUI::defaultStartOptionsMenu(){
	startScreenOptionSkyCombo->setItemSelectState( startScreenOptionSkyCombo->getListboxItemFromIndex( 0 ), true);
	startScreenOptionShadowsCombo->setItemSelectState( startScreenOptionShadowsCombo->getListboxItemFromIndex( 0 ), true);
}
//-------------------------------------------------------------------------------------
void GUI::builCharacterPreview(){
	demo = TUMBU::getInstance()->getDemo();
	if( !auxSceneMgrActive ) {
		auxSceneMgrActive = true;

		auxSceneMgr = mRoot->createSceneManager(Ogre::ST_GENERIC, "secondary");

		auxRobotNode = auxSceneMgr->getRootSceneNode()->createChildSceneNode("optionRobotNode");
		auxHeadNode = auxRobotNode->createChildSceneNode("optionRobotHeadNode");
		auxBodyNode = auxRobotNode->createChildSceneNode("optionRobotBodyNode");
		auxRightArmNode = auxRobotNode->createChildSceneNode("optionRobotRightArmNode");
		auxLeftArmNode = auxRobotNode->createChildSceneNode("optionRobotLeftArmNode");
		auxLegsNode = auxRobotNode->createChildSceneNode("optionRobotLegsNode");

		headPart = new Part( HEAD,			hero->head->setName, auxHeadNode,			auxSceneMgr );
		bodyPart = new Part( BODY,			hero->body->setName, auxBodyNode,			auxSceneMgr );
		rightArmPart = new Part( RIGHT_ARM,	hero->rightArm->setName, auxRightArmNode,	auxSceneMgr );
		leftArmPart = new Part( LEFT_ARM,	hero->leftArm->setName, auxLeftArmNode,	auxSceneMgr );
		legsPart = new Part( LEGS,			hero->legs->setName, auxLegsNode,			auxSceneMgr );

		headPart->build();
		bodyPart->build();
		rightArmPart->build();
		leftArmPart->build();
		legsPart->build();

		auxHeadNode->setPosition(		Ogre::Vector3( headPart->position ) );
		auxBodyNode->setPosition(		Ogre::Vector3( bodyPart->position ) );
		auxRightArmNode->setPosition(	Ogre::Vector3( rightArmPart->position ) );
		auxLeftArmNode->setPosition(	Ogre::Vector3( leftArmPart->position ) );
		auxLegsNode->setPosition(		Ogre::Vector3( legsPart->position ) );

		auxRobotNode->setPosition(0, 0, 0);

		Ogre::Light* luz = auxSceneMgr->createLight("luz_esc");
		luz->setPosition( Ogre::Vector3( 0, 0.8f, 1000) );

		auxTexture = mRoot->getTextureManager()->createManual(
			"RTT",
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			Ogre::TEX_TYPE_2D,
			1024,
			1024,
			0,
			Ogre::PF_R8G8B8,
			Ogre::TU_RENDERTARGET);
		renderTexture = auxTexture->getBuffer()->getRenderTarget();
 
		auxCamera = auxSceneMgr->createCamera("RTTCam");
		auxCamera->setPosition(0, 0.9f, 3);
		auxCamera->lookAt(0, 0.9f, 0);
		auxCamera->setNearClipDistance(0.001f);
		auxCamera->setFarClipDistance(1000);

		Ogre::Viewport *v = renderTexture->addViewport(auxCamera);
		v->setOverlaysEnabled(false);
		v->setClearEveryFrame(true);
		v->setBackgroundColour(Ogre::ColourValue::Black);

		CEGUI::Texture &guiTex = cegui->createTexture(auxTexture);
		CEGUI::Imageset &imageSet = CEGUI::ImagesetManager::getSingleton().create("RTTImageset", guiTex);

		imageSet.defineImage("RTTImage",
							 CEGUI::Point(0.0f, 0.0f),
							 CEGUI::Size(guiTex.getSize().d_width, guiTex.getSize().d_height),
							 CEGUI::Point(0.0f, 0.0f));
  
		optionInventoryRenderToTexture->setSize(CEGUI::UVector2(CEGUI::UDim(0.5f, 0), CEGUI::UDim(0.8f, 0)));

		optionInventoryRenderToTexture->setProperty("Image", CEGUI::PropertyHelper::imageToString( &imageSet.getImage("RTTImage") ));
 
		optionInventoryRenderToTexture->setAlwaysOnTop(true);
	}

}
//-------------------------------------------------------------------------------------
void GUI::hideAllOptions(){
	hideOptionStatus();
	hideOptionInventory();
	hideOptionSkills();
	hideOptionHelp();
}
//-------------------------------------------------------------------------------------
void GUI::hideOptionStatus(){
	optionsStatusWindow->setVisible(false);
	optionsStatusWindow->setAlwaysOnTop(false);
}
//-------------------------------------------------------------------------------------
void GUI::hideOptionInventory(){
	optionsInventoryWindow->setVisible(false);
	optionsInventoryWindow->setAlwaysOnTop(false);
}
//-------------------------------------------------------------------------------------
void GUI::hideOptionSkills(){
	optionsSkillsWindow->setVisible(false);
	optionsSkillsWindow->setAlwaysOnTop(false);
}
//-------------------------------------------------------------------------------------
void GUI::hideOptionHelp(){
	optionsHelpWindow->setVisible(false);
	optionsHelpWindow->setAlwaysOnTop(false);
}
//-------------------------------------------------------------------------------------
void GUI::setHero(Character* _hero){
	hero = _hero;
}
//-------------------------------------------------------------------------------------
void GUI::setEnemy(CharacterEnemy* _enemy){
	enemy = _enemy;
}
//-------------------------------------------------------------------------------------
/** EVENTOS */
//-------------------------------------------------------------------------------------
bool GUI::isOptionStatusOn(const CEGUI::EventArgs &e){
	if( !optionsStatusWindow->isVisible() ){
		hideAllOptions();
		optionsStatusWindow->setVisible(true);
		optionsStatusWindow->setAlwaysOnTop(true);


		/** HP */
		CEGUI::String hp;
		hp.append( Ogre::StringConverter::toString( (int) hero->hp) );
		hp.append(" / ");
		hp.append( Ogre::StringConverter::toString( (int) hero->getMaxHp()) );
		optionStatusHp->setText(hp);
		
		/** AP */
		CEGUI::String ap;
		ap.append( Ogre::StringConverter::toString( (int) hero->ap) );
		ap.append(" / ");
		ap.append( Ogre::StringConverter::toString( (int) hero->getMaxAp()) );
		optionStatusAp->setText(ap);

		/** Attack */
		CEGUI::String attack;
		attack.append( Ogre::StringConverter::toString( (int) hero->getMaxAttack()) );
		optionStatusAttack->setText(attack);

		/** Deffense */
		CEGUI::String deffense;
		deffense.append( Ogre::StringConverter::toString( (int) hero->getMaxDefense()) );
		optionStatusDeffense->setText(deffense);

		/** Velocity */
		CEGUI::String velocity;
		velocity.append( Ogre::StringConverter::toString( (int) hero->getMaxVelocity()) );
		optionStatusVelocity->setText(velocity);
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::isOptionInventoryOn(const CEGUI::EventArgs &e){
	demo = TUMBU::getInstance()->getDemo();

	if( !optionsInventoryWindow->isVisible() ){
		hideAllOptions();
		builCharacterPreview();

		optionsInventoryWindow->setVisible(true);
		optionsInventoryWindow->setAlwaysOnTop(true);
		
		CEGUI::ListboxItem* item;

		optionInventoryHeadCombo->resetList();
		for( unsigned int i = 0; i < hero->headList.size(); i++){
			item = new CEGUI::ListboxTextItem( hero->headList[i]->displayName, hero->headList[i]->id);
			optionInventoryHeadCombo->addItem( item );
			
			if( hero->headList[i]->id == hero->head->id ){
				optionInventoryHeadCombo->setItemSelectState(item, true);
			}
		}

		optionInventoryBodyCombo->resetList();		
		for( unsigned int i = 0; i < hero->bodyList.size(); i++){
			item = new CEGUI::ListboxTextItem( hero->bodyList[i]->displayName, hero->bodyList[i]->id);
			optionInventoryBodyCombo->addItem( item );

			if( hero->bodyList[i]->id == hero->body->id ){
				optionInventoryBodyCombo->setItemSelectState(item, true);
			}
		}

		optionInventoryRightArmCombo->resetList();
		for( unsigned int i = 0; i < hero->rightArmList.size(); i++){
			item = new CEGUI::ListboxTextItem(hero->rightArmList[i]->displayName, hero->rightArmList[i]->id);
			optionInventoryRightArmCombo->addItem( item );

			if( hero->rightArmList[i]->id == hero->rightArm->id ){
				optionInventoryRightArmCombo->setItemSelectState(item, true);
			}
		}

		optionInventoryLeftArmCombo->resetList();		
		for( unsigned int i = 0; i < hero->leftArmList.size(); i++){
			item = new CEGUI::ListboxTextItem( hero->leftArmList[i]->displayName, hero->leftArmList[i]->id);
			optionInventoryLeftArmCombo->addItem( item );

			if( hero->leftArmList[i]->id == hero->leftArm->id ){
				optionInventoryLeftArmCombo->setItemSelectState(item, true);
			}
		}

		optionInventoryLegsCombo->resetList();		
		for( unsigned int i = 0; i < hero->legsList.size(); i++){
			item = new CEGUI::ListboxTextItem( hero->legsList[i]->displayName, hero->legsList[i]->id);
			optionInventoryLegsCombo->addItem( item );

			if( hero->legsList[i]->id == hero->legs->id ){
				optionInventoryLegsCombo->setItemSelectState(item, true);
			}
		}
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::isOptionSkillsOn(const CEGUI::EventArgs &e){
	demo = TUMBU::getInstance()->getDemo();

	if( !optionsSkillsWindow->isVisible() ){

		hideAllOptions();
		optionsSkillsWindow->setVisible(true);
		optionsSkillsWindow->setAlwaysOnTop(true);

		float height = 0.01f;
		CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();
		Skill* _skill;

		CEGUI::Window *button;
	 
		// TODO ARRUMAR ISSO ABAIXO
		int loops = optionSkillsMovesListLabel->getChildCount();
		for( int i = 0; i < loops; i++){
			button = optionSkillsMovesListLabel->getChildAtIdx(i);

			if ( button->getType() == "TaharezLook/Button" ){
				optionSkillsMovesListLabel->removeChildWindow(i);
				wmgr.destroyWindow(button);
				button->removeAllEvents();
				loops -= 1;
				i--;
			}
		}

		std::list<Skill*>::iterator i = hero->skillList.begin();
		while ( i != hero->skillList.end() ){
			_skill = (*i);
			
			button = wmgr.createWindow( "TaharezLook/Button", "button_" + _skill->skillID);
			button->setPosition( CEGUI::UVector2( CEGUI::UDim( 0.01f, 0) , CEGUI::UDim( height, 0 ) ) );
			height += 0.09f;
			button->setSize( CEGUI::UVector2( CEGUI::UDim( 0.99f, 0) , CEGUI::UDim( 0.09f, 0 ) ) );
			button->setText( _skill->skillID );
			button->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUI::skillButtonClick, this));
			optionSkillsMovesListLabel->addChildWindow( button );

			i++;
			height += 0.01f;
		}
	}

	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::isOptionHelpOn(const CEGUI::EventArgs &e){
	if( !optionsHelpWindow->isVisible() ){
		hideAllOptions();
		optionsHelpWindow->setVisible(true);
		optionsHelpWindow->setAlwaysOnTop(true);
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::dialogOk(const CEGUI::EventArgs &e){
	lastActiveDialog->setAnswered(true);
	lastActiveDialog->setOk(true);
	showNextDialog( true );
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::dialogCancel(const CEGUI::EventArgs &e){
	lastActiveDialog->setAnswered(true);
	lastActiveDialog->setCancel(true);
	showNextDialog(true);
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::dialogShowPartOk(const CEGUI::EventArgs &e){
	lastActiveDialog->setAnswered(true);
	lastActiveDialog->setOk(true);
	showNextDialog(true);
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::continueTopConversation(const CEGUI::EventArgs &e){
	showNextDialog(true);
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::continueBottomConversation(const CEGUI::EventArgs &e){
	showNextDialog(true);
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::gameOverOk(const CEGUI::EventArgs &e){
	lastActiveDialog->setAnswered(true);
	lastActiveDialog->setOk(true);
	showNextDialog(true);

	rootLog->setVisible( false );
	TUMBU::getInstance()->finishDemo();
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::quit(const CEGUI::EventArgs &e){
	turnOptionOnOff();
	TUMBU::getInstance()->finishDemo();
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::gameStart(const CEGUI::EventArgs &e){
	hideStartMenu();
	TUMBU::getInstance()->initializeDemo();
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::gameOptions(const CEGUI::EventArgs &e){
	showStartOptionsMenu();
	loadStartOptionsMenu();
	hideStartMenu();
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::gameExit(const CEGUI::EventArgs &e){
	hideStartMenu();
	TUMBU::getInstance()->shutdown();
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::gameOptionsOk(const CEGUI::EventArgs &e){
	hideStartOptionsMenu();
	saveStartOptionsMenu();
	showStartMenu();
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::gameOptionsDefault(const CEGUI::EventArgs &e){
	defaultStartOptionsMenu();
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::gameOptionsCancel(const CEGUI::EventArgs &e){
	hideStartOptionsMenu();
	showStartMenu();
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::changeRobotHead(const CEGUI::EventArgs &e){
	unsigned int i;
	demo = TUMBU::getInstance()->getDemo();

	for( i = 0; i < hero->headList.size(); i++){
		if(	hero->headList[i]->id == optionInventoryHeadCombo->getSelectedItem()->getID() ){
			break;
		}
	}
	
	hero->unbuildParts();
	hero->head = hero->headList[i];
	hero->buildCharacter();
 	
	unbuildCharacterPreview();
	builCharacterPreview();

	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::changeRobotBody(const CEGUI::EventArgs &e){
	unsigned int i;
	demo = TUMBU::getInstance()->getDemo();

	for( i = 0; i < hero->bodyList.size(); i++){
		if(	hero->bodyList[i]->id == optionInventoryBodyCombo->getSelectedItem()->getID() ){
			break;
		}
	}
	
	hero->unbuildParts();
	hero->body = hero->bodyList[i];
	hero->buildCharacter();

	unbuildCharacterPreview();
	builCharacterPreview();

	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::changeRobotRightArm(const CEGUI::EventArgs &e){
	unsigned int i;
	demo = TUMBU::getInstance()->getDemo();

	for( i = 0; i < hero->rightArmList.size(); i++){
		if(	hero->rightArmList[i]->id == optionInventoryRightArmCombo->getSelectedItem()->getID() ){
			break;
		}
	}

	hero->unbuildParts();
	hero->rightArm = hero->rightArmList[i];
	hero->buildCharacter();

	unbuildCharacterPreview();
	builCharacterPreview();

	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::changeRobotLeftArm(const CEGUI::EventArgs &e){
	unsigned int i;
	demo = TUMBU::getInstance()->getDemo();

	for( i = 0; i < hero->leftArmList.size(); i++){
		if(	hero->leftArmList[i]->id == optionInventoryLeftArmCombo->getSelectedItem()->getID() ){
			break;
		}
	}
	
	hero->unbuildParts();
	hero->leftArm = hero->leftArmList[i];
	hero->buildCharacter();

	unbuildCharacterPreview();
	builCharacterPreview();

	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::changeRobotLegs(const CEGUI::EventArgs &e){
	unsigned int i;
	demo = TUMBU::getInstance()->getDemo();

	for( i = 0; i < hero->legsList.size(); i++){
		if(	hero->legsList[i]->id == optionInventoryLegsCombo->getSelectedItem()->getID() ){
			break;
		}
	}
	
	hero->unbuildParts();
	hero->legs = hero->legsList[i];
	hero->buildCharacter();

	unbuildCharacterPreview();
	builCharacterPreview();

	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::skillButtonClick(const CEGUI::EventArgs &e){
	const CEGUI::MouseEventArgs& we = static_cast<const CEGUI::MouseEventArgs&>(e);
	demo = TUMBU::getInstance()->getDemo();
	CEGUI::String buttonName = we.window->getName();

	Skill *skill;

	if( buttonName == "button_punch" ){
		skill = hero->punch;
	}else if( buttonName == "button_kick" ){
		skill = hero->kick;
	}else if( buttonName == "button_jyn" ){
		skill = hero->jyn;
	}
	
	Ogre::String text;

	text.append( Ogre::StringConverter::toString( skill->getDamage() ) );
	text.append( "\n" ).append( Ogre::StringConverter::toString( skill->getEnergyBalls() ) );
	text.append( "\n" ).append( Ogre::StringConverter::toString( skill->getAp() ) );
	text.append( "\n" ).append( Ogre::StringConverter::toString( skill->getLevel() ) );
	text.append( "\n" ).append( Ogre::StringConverter::toString( skill->getExperience() ) );
	text.append( "\n" ).append( Ogre::StringConverter::toString( skill->getExperienceNextLevel() ) );
		
	optionSkillsStatesValue->setText( text );

	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::startTutorialMode(const CEGUI::EventArgs &e){
	TUMBU::getInstance()->getDemo()->startTutorialMode();
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::stopTutorialMode(const CEGUI::EventArgs &e){
	TUMBU::getInstance()->getDemo()->stopTutorialMode();
	showNextDialog(true);
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::teste(const CEGUI::EventArgs &e){
	cout << "TESTEEEEEEEEEEEE " << endl;

	return true;
}
//-------------------------------------------------------------------------------------