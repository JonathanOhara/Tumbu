#include "GUI.h"
#include "TUMBU.h"
#include "Clock.h"

#include <MyGUI.h>
#include <MyGUI_OgrePlatform.h>
//-------------------------------------------------------------------------------------
GUI* GUI::instance = NULL;

namespace{
	const char* PREVIEW_TEXTURE = "TumbuPreviewRTT";
	const float SKILL_HIT_DURATION = 1.2f;

	std::string toString( int value ){
		return Ogre::StringConverter::toString( value );
	}

	MyGUI::Colour logColour( Log::LogType type ){
		switch( type ){
		case Log::WARNING:	return MyGUI::Colour( 1.0f, 1.0f, 0.0f );
		case Log::FAILED:	return MyGUI::Colour( 1.0f, 0.3f, 0.3f );
		case Log::LEVEL_UP:	return MyGUI::Colour( 0.4f, 0.7f, 1.0f );
		case Log::GREEN:	return MyGUI::Colour( 0.3f, 1.0f, 0.3f );
		default:			return MyGUI::Colour::White;
		}
	}
}
//-------------------------------------------------------------------------------------
GUI::GUI( Ogre::RenderWindow* window, Ogre::Root *root ){
	mWindow				= window;
	mRoot				= root;
	hero				= NULL;
	enemy				= NULL;
	hasActiveDialog		= false;
	waitingUserContinue	= false;
	startMenuVisible	= false;
	lastConversationTop	= false;
	mouseX = mouseY = mouseZ = 0;

	previewSceneMgr		= NULL;
	previewRoot			= NULL;
	previewCamera		= NULL;
	previewEntity		= NULL;
	previewRotating		= false;

	mPlatform = new MyGUI::OgrePlatform();
	mPlatform->initialise( window, TUMBU::getInstance()->mSceneMgr, "MyGUI", TUMBU::getInstance()->workPath + "MyGUI.log" );
	mGui = new MyGUI::Gui();
	mGui->initialise( "Tumbu_Core.xml" );

	// The OS cursor is used (SDL); MyGUI's own pointer would draw a second one.
	MyGUI::PointerManager::getInstance().setVisible( false );

	loadLayouts();
	connectEvents();
}
//-------------------------------------------------------------------------------------
GUI::~GUI(void){
	clearPreview();

	// MyGUI first: it holds a wrapper around the preview texture and releases it on shutdown.
	mGui->shutdown();
	delete mGui;
	mPlatform->shutdown();
	delete mPlatform;

	if( previewSceneMgr != NULL ){
		if( previewTexture ){
			previewTexture->getBuffer()->getRenderTarget()->removeAllViewports();
			if( Ogre::TextureManager::getSingleton().resourceExists( PREVIEW_TEXTURE, "MyGUI" ) ){
				Ogre::TextureManager::getSingleton().remove( previewTexture );
			}
			previewTexture.reset();
		}
		Ogre::RTShader::ShaderGenerator::getSingleton().removeSceneManager( previewSceneMgr );
		mRoot->destroySceneManager( previewSceneMgr );
	}
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
template<class T> T* GUI::widget( const std::string &name ){
	return mGui->findWidget<T>( name );
}
//------------------------------------------------------------------------------------- setup
void GUI::loadLayouts(void){
	MyGUI::LayoutManager &layouts = MyGUI::LayoutManager::getInstance();
	layouts.loadLayout( "StartScreen.layout" );
	layouts.loadLayout( "Battle.layout" );
	layouts.loadLayout( "Dialogs.layout" );
	layouts.loadLayout( "PauseMenu.layout" );

	startMenu			= widget<MyGUI::Widget>( "StartMenu" );
	optionsWindow		= widget<MyGUI::Window>( "OptionsWindow" );
	skyCombo			= widget<MyGUI::ComboBox>( "SkyCombo" );
	shadowsCombo		= widget<MyGUI::ComboBox>( "ShadowsCombo" );
	frameLimitCombo		= widget<MyGUI::ComboBox>( "FrameLimitCombo" );

	battleHud			= widget<MyGUI::Widget>( "BattleHud" );
	heroHpBar			= widget<MyGUI::ProgressBar>( "HeroHpBar" );
	heroApBar			= widget<MyGUI::ProgressBar>( "HeroApBar" );
	enemyHpBar			= widget<MyGUI::ProgressBar>( "EnemyHpBar" );
	enemyApBar			= widget<MyGUI::ProgressBar>( "EnemyApBar" );
	heroHpValue			= widget<MyGUI::TextBox>( "HeroHpValue" );
	heroApValue			= widget<MyGUI::TextBox>( "HeroApValue" );
	loadingText			= widget<MyGUI::TextBox>( "LoadingText" );
	for( int i = 0; i < 4; i++ ){
		logLines[i] = widget<MyGUI::TextBox>( "Log" + toString( i + 1 ) );
		logLines[i]->setCaption( "" );
	}

	alertWindow			= widget<MyGUI::Window>( "AlertWindow" );
	confirmWindow		= widget<MyGUI::Window>( "ConfirmWindow" );
	showPartWindow		= widget<MyGUI::Window>( "ShowPartWindow" );
	conversationTop		= widget<MyGUI::Window>( "ConversationTop" );
	conversationBottom	= widget<MyGUI::Window>( "ConversationBottom" );

	pauseMenu			= widget<MyGUI::Widget>( "PauseMenu" );
	statusPanel			= widget<MyGUI::Widget>( "StatusPanel" );
	inventoryPanel		= widget<MyGUI::Widget>( "InventoryPanel" );
	skillsPanel			= widget<MyGUI::Widget>( "SkillsPanel" );
	helpPanel			= widget<MyGUI::Widget>( "HelpPanel" );
	skillsList			= widget<MyGUI::Widget>( "SkillsList" );
	skillDetails		= widget<MyGUI::Window>( "SkillDetails" );
	clockText			= widget<MyGUI::TextBox>( "Clock" );
	partCombos[HEAD]		= widget<MyGUI::ComboBox>( "HeadCombo" );
	partCombos[BODY]		= widget<MyGUI::ComboBox>( "BodyCombo" );
	partCombos[RIGHT_ARM]	= widget<MyGUI::ComboBox>( "RightArmCombo" );
	partCombos[LEFT_ARM]	= widget<MyGUI::ComboBox>( "LeftArmCombo" );
	partCombos[LEGS]		= widget<MyGUI::ComboBox>( "LegsCombo" );

	widget<MyGUI::EditBox>( "SkillStatsLabels" )->setCaption( "Damage:\nEnergy Balls:\nAP cost:\nLevel:\nExperience:\nNext Level:" );
	widget<MyGUI::EditBox>( "HelpText" )->setCaption(
		"KEYBOARD\n"
		"  W A S D - Move            Left Shift - Run\n"
		"  Q / E - Rotate camera     Mouse - Look around, wheel zooms\n"
		"  U or 1 - Kick             O or 2 - Punch\n"
		"  I or 3 - Jyn: press to call energy balls, again to concentrate, again to attack\n"
		"  P or Left Ctrl - Guard\n"
		"  Space / Enter - OK in dialogs     Esc - Cancel / pause menu\n"
		"\n"
		"GAMEPAD\n"
		"  Left stick / D-pad - Move           LB - Run\n"
		"  Triggers / right stick - Rotate camera\n"
		"  A - Kick     X - Punch     Y - Jyn     RB - Guard\n"
		"  A - OK in dialogs     B - Cancel     Start - Pause menu" );

	skyCombo->addItem( "Low (sky dome)" );
	skyCombo->addItem( "High (day/night sky)" );
	shadowsCombo->addItem( "None" );
	shadowsCombo->addItem( "Modulative" );
	shadowsCombo->addItem( "Additive" );
	frameLimitCombo->addItem( "VSync (monitor)", -1 );
	frameLimitCombo->addItem( "144 FPS", 144 );
	frameLimitCombo->addItem( "72 FPS", 72 );
	frameLimitCombo->addItem( "60 FPS", 60 );
	frameLimitCombo->addItem( "Unlimited", 0 );

	hideDialogWidgets();
	pauseMenu->setVisible( false );
	battleHud->setVisible( false );
	startMenu->setVisible( false );
}
//-------------------------------------------------------------------------------------
void GUI::connectEvents(void){
	widget<MyGUI::Button>( "PlayButton" )->eventMouseButtonClick			+= MyGUI::newDelegate( this, &GUI::onPlayClick );
	widget<MyGUI::Button>( "OptionsButton" )->eventMouseButtonClick			+= MyGUI::newDelegate( this, &GUI::onOptionsClick );
	widget<MyGUI::Button>( "ExitButton" )->eventMouseButtonClick			+= MyGUI::newDelegate( this, &GUI::onExitClick );
	widget<MyGUI::Button>( "OptionsOkButton" )->eventMouseButtonClick		+= MyGUI::newDelegate( this, &GUI::onOptionsOk );
	widget<MyGUI::Button>( "OptionsDefaultButton" )->eventMouseButtonClick	+= MyGUI::newDelegate( this, &GUI::onOptionsDefault );
	widget<MyGUI::Button>( "OptionsCancelButton" )->eventMouseButtonClick	+= MyGUI::newDelegate( this, &GUI::onOptionsCancel );

	widget<MyGUI::Button>( "AlertOkButton" )->eventMouseButtonClick				+= MyGUI::newDelegate( this, &GUI::onDialogOk );
	widget<MyGUI::Button>( "ConfirmOkButton" )->eventMouseButtonClick			+= MyGUI::newDelegate( this, &GUI::onDialogOk );
	widget<MyGUI::Button>( "ConfirmCancelButton" )->eventMouseButtonClick		+= MyGUI::newDelegate( this, &GUI::onDialogCancel );
	widget<MyGUI::Button>( "ShowPartOkButton" )->eventMouseButtonClick			+= MyGUI::newDelegate( this, &GUI::onDialogOk );
	widget<MyGUI::Button>( "ConversationTopContinue" )->eventMouseButtonClick	+= MyGUI::newDelegate( this, &GUI::onDialogOk );
	widget<MyGUI::Button>( "ConversationBottomContinue" )->eventMouseButtonClick += MyGUI::newDelegate( this, &GUI::onDialogOk );

	const char* tabs[] = { "StatusTab", "InventoryTab", "SkillsTab", "HelpTab", "QuitTab" };
	for( size_t i = 0; i < 5; i++ ){
		widget<MyGUI::Button>( tabs[i] )->eventMouseButtonClick += MyGUI::newDelegate( this, &GUI::onPauseTab );
	}
	for( int i = 0; i < 5; i++ ){
		partCombos[i]->setUserData( MyGUI::Any( i ) );
		partCombos[i]->eventComboAccept += MyGUI::newDelegate( this, &GUI::onPartComboAccept );
	}
}
//-------------------------------------------------------------------------------------
void GUI::reset(void){
	while( !dialogList.empty() ){
		dialogList.pop();
	}
	hasActiveDialog			= false;
	waitingUserContinue		= false;
	lastConversationTop		= false;
	lastConversationName	= "";
	hero					= NULL;
	enemy					= NULL;

	logList.clear();
	for( int i = 0; i < 4; i++ ){
		logLines[i]->setCaption( "" );
	}
	for( std::list<SkillHitEntry>::iterator it = skillHits.begin(); it != skillHits.end(); ++it ){
		mGui->destroyWidget( it->label );
	}
	skillHits.clear();

	hideDialogWidgets();
	hideBattleLayout();
	pauseMenu->setVisible( false );
	setPreviewActive( false );
	clearPreview();
	stopLoad();
}
//-------------------------------------------------------------------------------------
void GUI::initOptions(void){
}
//-------------------------------------------------------------------------------------
bool GUI::isOptionOn(void){
	return pauseMenu->getVisible();
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
void GUI::hideDialogWidgets(void){
	alertWindow->setVisible( false );
	confirmWindow->setVisible( false );
	showPartWindow->setVisible( false );
	conversationTop->setVisible( false );
	conversationBottom->setVisible( false );
}
//-------------------------------------------------------------------------------------
void GUI::showDialogWidgets( const DialogEntry &dialog ){
	hideDialogWidgets();
	setPreviewActive( false );

	switch( dialog.type ){
	case DialogEntry::ALERT:
		alertWindow->setCaption( dialog.title );
		widget<MyGUI::EditBox>( "AlertText" )->setCaption( dialog.message );
		alertWindow->setVisible( true );
		break;

	case DialogEntry::CONFIRM:
		confirmWindow->setCaption( dialog.title );
		widget<MyGUI::EditBox>( "ConfirmText" )->setCaption( dialog.message );
		confirmWindow->setVisible( true );
		break;

	case DialogEntry::SHOW_PART:{
		dialog.part->loadParameters();
		std::string text = dialog.message +
			"\n\nHP: " + toString( (int) dialog.part->hp ) +
			"\nAP: " + toString( (int) dialog.part->ap ) +
			"\nAttack: " + toString( (int) dialog.part->attack ) +
			"\nDefense: " + toString( (int) dialog.part->defense ) +
			"\nVelocity: " + toString( (int) dialog.part->velocity );
		showPartWindow->setCaption( dialog.title );
		widget<MyGUI::EditBox>( "ShowPartText" )->setCaption( text );
		showPreviewPart( dialog.part );
		widget<MyGUI::ImageBox>( "ShowPartImage" )->setImageTexture( PREVIEW_TEXTURE );
		showPartWindow->setVisible( true );
		break;
	}

	case DialogEntry::CONVERSATION:{
		// A new speaker switches to the other panel, like the 2011 game.
		if( lastConversationName != dialog.title ){
			lastConversationTop = !lastConversationTop;
			lastConversationName = dialog.title;
		}
		std::string prefix = lastConversationTop ? "ConversationTop" : "ConversationBottom";
		MyGUI::Window *panel = lastConversationTop ? conversationTop : conversationBottom;
		panel->setCaption( dialog.title );
		widget<MyGUI::EditBox>( prefix + "Text" )->setCaption( dialog.message );
		widget<MyGUI::ImageBox>( prefix + "Face" )->setItemName( dialog.image );
		panel->setVisible( true );
		break;
	}
	}
}
//-------------------------------------------------------------------------------------
void GUI::showNextDialog( bool ok ){
	if( !dialogList.empty() ){
		TUMBU::getInstance()->setGameState( TumbuEnums::IN_DIALOG );
		activeDialog = dialogList.front();
		dialogList.pop();
		hasActiveDialog = true;
		waitingUserContinue = true;
		showDialogWidgets( activeDialog );
		setMouseCursorVisibility( true );
		Ogre::LogManager::getSingleton().logMessage( "[GUI] dialog: " + activeDialog.title + " - " + activeDialog.message );
	}else{
		hasActiveDialog = false;
		waitingUserContinue = false;
		lastConversationTop = false;
		lastConversationName = "";
		hideDialogWidgets();
		setPreviewActive( false );
		clearPreview();
		if( TUMBU::getInstance()->getGameState() == TumbuEnums::IN_DIALOG ){
			TUMBU::getInstance()->setGameState( TumbuEnums::PLAYING );
			setMouseCursorVisibility( false );
		}
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
	LogEntry entry;
	entry.message = message;
	entry.type = logType;
	entry.remaining = duration;
	logList.push_back( entry );
	while( logList.size() > 4 ){
		logList.pop_front();
	}
}
//-------------------------------------------------------------------------------------
void GUI::updateLog( float time ){
	std::list<LogEntry>::iterator it = logList.begin();
	while( it != logList.end() ){
		it->remaining -= time;
		if( it->remaining <= 0 ){
			it = logList.erase( it );
		}else{
			++it;
		}
	}
	// Newest message on the bottom line.
	int line = 4 - (int) logList.size();
	for( int i = 0; i < line; i++ ){
		logLines[i]->setCaption( "" );
	}
	for( std::list<LogEntry>::iterator log = logList.begin(); log != logList.end(); ++log, ++line ){
		logLines[line]->setCaption( log->message );
		logLines[line]->setTextColour( logColour( log->type ) );
	}
}
//-------------------------------------------------------------------------------------
void GUI::addSkillHit( const Ogre::Vector3 &position, const Ogre::String &message ){
	SkillHitEntry hit;
	hit.position = position + Ogre::Vector3( 0, 2.0f, 0 );	// above the robot's head
	hit.age = 0;
	hit.label = mGui->createWidget<MyGUI::TextBox>( "TextBox", MyGUI::IntCoord( 0, 0, 200, 40 ), MyGUI::Align::Default, "Info" );
	hit.label->setFontName( "Tumbu.Hit" );
	hit.label->setTextAlign( MyGUI::Align::Center );
	hit.label->setTextShadow( true );
	hit.label->setTextColour( MyGUI::Colour( 1.0f, 0.85f, 0.2f ) );
	hit.label->setNeedMouseFocus( false );
	hit.label->setCaption( message );
	hit.label->setVisible( false );
	skillHits.push_back( hit );
}
//-------------------------------------------------------------------------------------
void GUI::updateSkillHits( float time ){
	Demo *demo = TUMBU::getInstance()->getDemo();
	Ogre::Camera *camera = TUMBU::getInstance()->mCamera;

	std::list<SkillHitEntry>::iterator it = skillHits.begin();
	while( it != skillHits.end() ){
		it->age += time;
		if( it->age >= SKILL_HIT_DURATION || demo == NULL ){
			mGui->destroyWidget( it->label );
			it = skillHits.erase( it );
			continue;
		}
		// Project the 3D position to the screen; the number rises and fades out.
		Ogre::Vector3 world = it->position + Ogre::Vector3( 0, it->age * 0.8f, 0 );
		Ogre::Vector3 clip = camera->getProjectionMatrix() * ( camera->getViewMatrix() * world );
		bool inFront = ( camera->getViewMatrix() * world ).z < 0;
		int x = (int) ( ( clip.x * 0.5f + 0.5f ) * mWindow->getWidth() ) - 100;
		int y = (int) ( ( 0.5f - clip.y * 0.5f ) * mWindow->getHeight() ) - 20;
		it->label->setPosition( x, y );
		it->label->setAlpha( 1.0f - it->age / SKILL_HIT_DURATION );
		it->label->setVisible( inFront );
		++it;
	}
}
//-------------------------------------------------------------------------------------
void GUI::updateHud(void){
	if( hero != NULL ){
		heroHpBar->setProgressPosition( (size_t) ( Ogre::Math::Clamp( hero->hpPercent, 0.0f, 1.0f ) * 1000 ) );
		heroApBar->setProgressPosition( (size_t) ( Ogre::Math::Clamp( hero->apPercent, 0.0f, 1.0f ) * 1000 ) );
		heroHpValue->setCaption( toString( (int) hero->hp ) + " / " + toString( (int) hero->getMaxHp() ) );
		heroApValue->setCaption( toString( (int) hero->ap ) + " / " + toString( (int) hero->getMaxAp() ) );
	}
	if( enemy != NULL ){
		enemyHpBar->setProgressPosition( (size_t) ( Ogre::Math::Clamp( enemy->hpPercent, 0.0f, 1.0f ) * 1000 ) );
		enemyApBar->setProgressPosition( (size_t) ( Ogre::Math::Clamp( enemy->apPercent, 0.0f, 1.0f ) * 1000 ) );
	}
}
//-------------------------------------------------------------------------------------
void GUI::startLoad( const Ogre::String &message ){
	loadingText->setCaption( message );
	loadingText->setVisible( true );
}
//-------------------------------------------------------------------------------------
void GUI::stopLoad(void){
	loadingText->setVisible( false );
}
//-------------------------------------------------------------------------------------
void GUI::showBattleLayout(void){
	battleHud->setVisible( true );
}
//-------------------------------------------------------------------------------------
void GUI::hideBattleLayout(void){
	battleHud->setVisible( false );
}
//-------------------------------------------------------------------------------------
void GUI::showStartMenu(void){
	startMenuVisible = true;
	startMenu->setVisible( true );
	optionsWindow->setVisible( false );
	setMouseCursorVisibility( true );
}
//-------------------------------------------------------------------------------------
void GUI::hideStartMenu(void){
	startMenuVisible = false;
	startMenu->setVisible( false );
	optionsWindow->setVisible( false );
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
//------------------------------------------------------------------------------------- start screen options
void GUI::showOptions(void){
	loadOptionsIntoWidgets();
	startMenu->setVisible( false );
	optionsWindow->setVisible( true );
}
//-------------------------------------------------------------------------------------
void GUI::loadOptionsIntoWidgets(void){
	TUMBU *tumbu = TUMBU::getInstance();
	skyCombo->setIndexSelected( tumbu->getSkyQuality() == 1 ? 1 : 0 );

	switch( tumbu->getShadowTechnique() ){
	case Ogre::SHADOWTYPE_TEXTURE_MODULATIVE:	shadowsCombo->setIndexSelected( 1 ); break;
	case Ogre::SHADOWTYPE_TEXTURE_ADDITIVE:		shadowsCombo->setIndexSelected( 2 ); break;
	default:									shadowsCombo->setIndexSelected( 0 ); break;
	}

	frameLimitCombo->setIndexSelected( 0 );
	for( size_t i = 0; i < frameLimitCombo->getItemCount(); i++ ){
		if( *frameLimitCombo->getItemDataAt<int>( i ) == tumbu->getFrameLimit() ){
			frameLimitCombo->setIndexSelected( i );
		}
	}
}
//-------------------------------------------------------------------------------------
void GUI::applyOptionsFromWidgets(void){
	TUMBU *tumbu = TUMBU::getInstance();
	tumbu->setSkyQuality( skyCombo->getIndexSelected() == 1 ? 1 : 0 );
	tumbu->setShadowPreset( (int) shadowsCombo->getIndexSelected() );
	tumbu->setFrameLimit( *frameLimitCombo->getItemDataAt<int>( frameLimitCombo->getIndexSelected() ) );
	tumbu->saveOptions();
}
//------------------------------------------------------------------------------------- pause menu
void GUI::togglePauseMenu(void){
	if( isWaitingUserToContinue() ){
		return;
	}
	TUMBU *tumbu = TUMBU::getInstance();
	if( pauseMenu->getVisible() ){
		pauseMenu->setVisible( false );
		setPreviewActive( false );
		clearPreview();
		if( dialogList.empty() ){
			tumbu->setGameState( TumbuEnums::PLAYING );
			setMouseCursorVisibility( false );
		}
	}else if( tumbu->isPlaying() ){
		clockText->setCaption( Clock::getInstance()->getClockFormated() );
		pauseMenu->setVisible( true );
		showPauseTab( statusPanel );
		tumbu->setGameState( TumbuEnums::PAUSED );
		setMouseCursorVisibility( true );
	}
}
//-------------------------------------------------------------------------------------
void GUI::showPauseTab( MyGUI::Widget *panel ){
	statusPanel->setVisible( panel == statusPanel );
	inventoryPanel->setVisible( panel == inventoryPanel );
	skillsPanel->setVisible( panel == skillsPanel );
	helpPanel->setVisible( panel == helpPanel );

	setPreviewActive( panel == inventoryPanel );
	if( panel == statusPanel )		fillStatus();
	if( panel == inventoryPanel )	fillInventory();
	if( panel == skillsPanel )		fillSkills();
}
//-------------------------------------------------------------------------------------
void GUI::fillStatus(void){
	if( hero == NULL ) return;
	widget<MyGUI::TextBox>( "StatusHp" )->setCaption( toString( (int) hero->hp ) + " / " + toString( (int) hero->getMaxHp() ) );
	widget<MyGUI::TextBox>( "StatusAp" )->setCaption( toString( (int) hero->ap ) + " / " + toString( (int) hero->getMaxAp() ) );
	widget<MyGUI::TextBox>( "StatusAttack" )->setCaption( toString( (int) hero->getMaxAttack() ) );
	widget<MyGUI::TextBox>( "StatusDefense" )->setCaption( toString( (int) hero->getMaxDefense() ) );
	widget<MyGUI::TextBox>( "StatusVelocity" )->setCaption( toString( (int) hero->getMaxVelocity() ) );
}
//-------------------------------------------------------------------------------------
void GUI::fillInventory(void){
	if( hero == NULL ) return;
	std::vector<Part*>* lists[5] = { &hero->headList, &hero->bodyList, &hero->rightArmList, &hero->leftArmList, &hero->legsList };
	Part* equipped[5] = { hero->head, hero->body, hero->rightArm, hero->leftArm, hero->legs };

	for( int type = 0; type < 5; type++ ){
		MyGUI::ComboBox *combo = partCombos[type];
		combo->removeAllItems();
		for( size_t i = 0; i < lists[type]->size(); i++ ){
			Part *part = (*lists[type])[i];
			combo->addItem( part->displayName + " (" + part->setName + ")" );
			if( part == equipped[type] ){
				combo->setIndexSelected( i );
			}
		}
	}
	showPreviewRobot();
	widget<MyGUI::ImageBox>( "InventoryPreview" )->setImageTexture( PREVIEW_TEXTURE );
}
//-------------------------------------------------------------------------------------
void GUI::changePart( int partType, size_t index ){
	std::vector<Part*>* lists[5] = { &hero->headList, &hero->bodyList, &hero->rightArmList, &hero->leftArmList, &hero->legsList };
	if( hero == NULL || index >= lists[partType]->size() ){
		return;
	}
	Part *part = (*lists[partType])[index];

	hero->unbuildParts();
	switch( partType ){
	case HEAD:		hero->head = part; break;
	case BODY:		hero->body = part; break;
	case RIGHT_ARM:	hero->rightArm = part; break;
	case LEFT_ARM:	hero->leftArm = part; break;
	case LEGS:		hero->legs = part; break;
	}
	hero->buildCharacter();
	showPreviewRobot();
}
//-------------------------------------------------------------------------------------
void GUI::fillSkills(void){
	if( hero == NULL ) return;
	while( skillsList->getChildCount() > 0 ){
		mGui->destroyWidget( skillsList->getChildAt( 0 ) );
	}
	float top = 0;
	for( std::list<Skill*>::iterator it = hero->skillList.begin(); it != hero->skillList.end(); ++it ){
		MyGUI::Button *button = skillsList->createWidgetReal<MyGUI::Button>( "Button", MyGUI::FloatCoord( 0, top, 1, 0.14f ), MyGUI::Align::Default );
		button->setCaption( (*it)->skillID );
		button->setFontName( "Tumbu.Button" );
		button->setUserString( "skill", (*it)->skillID );
		button->eventMouseButtonClick += MyGUI::newDelegate( this, &GUI::onSkillClick );
		top += 0.17f;
	}
	if( !hero->skillList.empty() ){
		showSkillDetails( hero->skillList.front()->skillID );
	}
}
//-------------------------------------------------------------------------------------
void GUI::showSkillDetails( const std::string &skillID ){
	Skill *skill = NULL;
	for( std::list<Skill*>::iterator it = hero->skillList.begin(); it != hero->skillList.end(); ++it ){
		if( (*it)->skillID == skillID ){
			skill = *it;
		}
	}
	if( skill == NULL ) return;
	skillDetails->setCaption( skillID );
	widget<MyGUI::EditBox>( "SkillStatsValues" )->setCaption(
		toString( skill->getDamage() ) + "\n" +
		toString( skill->getEnergyBalls() ) + "\n" +
		toString( skill->getAp() ) + "\n" +
		toString( skill->getLevel() ) + "\n" +
		toString( skill->getExperience() ) + "\n" +
		toString( skill->getExperienceNextLevel() ) );
}
//------------------------------------------------------------------------------------- 3D preview
void GUI::createPreviewScene(void){
	if( previewSceneMgr != NULL ){
		return;
	}
	previewSceneMgr = mRoot->createSceneManager( Ogre::SMT_DEFAULT, "PreviewScene" );
	Ogre::RTShader::ShaderGenerator::getSingleton().addSceneManager( previewSceneMgr );
	previewSceneMgr->setAmbientLight( Ogre::ColourValue( 0.55f, 0.55f, 0.55f ) );

	Ogre::Light *light = previewSceneMgr->createLight( "PreviewLight" );
	light->setType( Ogre::Light::LT_DIRECTIONAL );
	Ogre::SceneNode *lightNode = previewSceneMgr->getRootSceneNode()->createChildSceneNode();
	lightNode->attachObject( light );
	lightNode->setDirection( Ogre::Vector3( -0.3f, -0.4f, -1.0f ), Ogre::Node::TS_WORLD );

	previewCamera = previewSceneMgr->createCamera( "PreviewCamera" );
	previewCamera->setNearClipDistance( 0.01f );
	previewCamera->setAspectRatio( 1.0f );
	Ogre::SceneNode *cameraNode = previewSceneMgr->getRootSceneNode()->createChildSceneNode( "PreviewCameraNode" );
	cameraNode->attachObject( previewCamera );

	previewRoot = previewSceneMgr->getRootSceneNode()->createChildSceneNode( "PreviewRoot" );

	// Rendered into a texture in the MyGUI group so image widgets can show it by name.
	previewTexture = Ogre::TextureManager::getSingleton().createManual( PREVIEW_TEXTURE, "MyGUI",
		Ogre::TEX_TYPE_2D, 512, 512, 0, Ogre::PF_BYTE_RGBA, Ogre::TU_RENDERTARGET );
	Ogre::RenderTarget *target = previewTexture->getBuffer()->getRenderTarget();
	Ogre::Viewport *viewport = target->addViewport( previewCamera );
	viewport->setOverlaysEnabled( false );
	viewport->setClearEveryFrame( true );
	viewport->setBackgroundColour( Ogre::ColourValue( 0.05f, 0.07f, 0.12f ) );
	viewport->setMaterialScheme( Ogre::MSN_SHADERGEN );
	target->setAutoUpdated( false );
}
//-------------------------------------------------------------------------------------
void GUI::clearPreview(void){
	for( size_t i = 0; i < previewParts.size(); i++ ){
		previewParts[i]->unbuild();
		delete previewParts[i];
	}
	previewParts.clear();
	if( previewEntity != NULL ){
		previewSceneMgr->destroyEntity( previewEntity );
		previewEntity = NULL;
	}
	if( previewRoot != NULL ){
		previewRoot->removeAndDestroyAllChildren();
		previewRoot->resetOrientation();
	}
}
//-------------------------------------------------------------------------------------
void GUI::setPreviewActive( bool active ){
	if( previewTexture ){
		previewTexture->getBuffer()->getRenderTarget()->setAutoUpdated( active );
	}
	previewRotating = active;
}
//-------------------------------------------------------------------------------------
void GUI::showPreviewRobot(void){
	createPreviewScene();
	clearPreview();

	Part* equipped[5] = { hero->head, hero->body, hero->rightArm, hero->leftArm, hero->legs };
	for( int type = 0; type < 5; type++ ){
		Ogre::SceneNode *node = previewRoot->createChildSceneNode();
		Part *part = new Part( type, equipped[type]->setName, node, previewSceneMgr );
		part->build();
		node->setPosition( part->position );
		previewParts.push_back( part );
	}
	previewCamera->getParentSceneNode()->setPosition( 0, 1.0f, 3.2f );
	previewCamera->getParentSceneNode()->lookAt( Ogre::Vector3( 0, 1.0f, 0 ), Ogre::Node::TS_WORLD );
	setPreviewActive( true );
}
//-------------------------------------------------------------------------------------
void GUI::showPreviewPart( Part *part ){
	createPreviewScene();
	clearPreview();

	previewEntity = previewSceneMgr->createEntity( part->meshName );
	Ogre::SceneNode *node = previewRoot->createChildSceneNode();
	node->attachObject( previewEntity );
	// Centre the part and frame it with the camera.
	Ogre::AxisAlignedBox bounds = previewEntity->getBoundingBox();
	node->setPosition( -bounds.getCenter() );
	Ogre::Real radius = std::max( 0.2f, bounds.getHalfSize().length() );
	previewCamera->getParentSceneNode()->setPosition( 0, 0, radius * 2.6f );
	previewCamera->getParentSceneNode()->lookAt( Ogre::Vector3::ZERO, Ogre::Node::TS_WORLD );
	setPreviewActive( true );
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
//------------------------------------------------------------------------------------- MyGUI events
void GUI::onPlayClick( MyGUI::Widget *sender ){
	gameStart();
}
void GUI::onOptionsClick( MyGUI::Widget *sender ){
	showOptions();
}
void GUI::onExitClick( MyGUI::Widget *sender ){
	gameExit();
}
void GUI::onOptionsOk( MyGUI::Widget *sender ){
	applyOptionsFromWidgets();
	showStartMenu();
}
void GUI::onOptionsDefault( MyGUI::Widget *sender ){
	skyCombo->setIndexSelected( 0 );
	shadowsCombo->setIndexSelected( 0 );
	frameLimitCombo->setIndexSelected( 0 );
}
void GUI::onOptionsCancel( MyGUI::Widget *sender ){
	showStartMenu();
}
void GUI::onDialogOk( MyGUI::Widget *sender ){
	answerDialog( true );
}
void GUI::onDialogCancel( MyGUI::Widget *sender ){
	answerDialog( false );
}
void GUI::onPauseTab( MyGUI::Widget *sender ){
	const std::string &name = sender->getName();
	if( name == "StatusTab" )			showPauseTab( statusPanel );
	else if( name == "InventoryTab" )	showPauseTab( inventoryPanel );
	else if( name == "SkillsTab" )		showPauseTab( skillsPanel );
	else if( name == "HelpTab" )		showPauseTab( helpPanel );
	else if( name == "QuitTab" ){
		togglePauseMenu();
		TUMBU::getInstance()->finishDemo();
	}
}
void GUI::onSkillClick( MyGUI::Widget *sender ){
	showSkillDetails( std::string( sender->getUserString( "skill" ) ) );
}
void GUI::onPartComboAccept( MyGUI::ComboBox *sender, size_t index ){
	changePart( *sender->getUserData<int>(), index );
}
//------------------------------------------------------------------------------------- frame and input
bool GUI::frameRenderingQueued( const Ogre::FrameEvent &evt ){
	if( previewRotating && previewRoot != NULL ){
		previewRoot->yaw( Ogre::Radian( 0.5f * evt.timeSinceLastFrame ) );
	}
	if( TUMBU::getInstance()->isPlaying() ){
		updateHud();
		updateLog( evt.timeSinceLastFrame );
	}
	updateSkillHits( TUMBU::getInstance()->isPlaying() ? evt.timeSinceLastFrame : 0 );
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
	}else if( key == TumbuInput::KEY_ESCAPE ){
		togglePauseMenu();
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::keyReleased( const OgreBites::KeyboardEvent &evt ){
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::mouseMoved( const OgreBites::MouseMotionEvent &evt ){
	mouseX = evt.x;
	mouseY = evt.y;
	MyGUI::InputManager::getInstance().injectMouseMove( mouseX, mouseY, mouseZ );
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::mouseWheelRolled( const OgreBites::MouseWheelEvent &evt ){
	mouseZ += evt.y * 120;
	MyGUI::InputManager::getInstance().injectMouseMove( mouseX, mouseY, mouseZ );
	return true;
}
//-------------------------------------------------------------------------------------
static MyGUI::MouseButton toMyGuiButton( unsigned char button ){
	switch( button ){
	case OgreBites::BUTTON_RIGHT:	return MyGUI::MouseButton::Right;
	case OgreBites::BUTTON_MIDDLE:	return MyGUI::MouseButton::Middle;
	default:						return MyGUI::MouseButton::Left;
	}
}
//-------------------------------------------------------------------------------------
bool GUI::mousePressed( const OgreBites::MouseButtonEvent &evt ){
	mouseX = evt.x;
	mouseY = evt.y;
	MyGUI::InputManager::getInstance().injectMousePress( mouseX, mouseY, toMyGuiButton( evt.button ) );
	return true;
}
//-------------------------------------------------------------------------------------
bool GUI::mouseReleased( const OgreBites::MouseButtonEvent &evt ){
	mouseX = evt.x;
	mouseY = evt.y;
	MyGUI::InputManager::getInstance().injectMouseRelease( mouseX, mouseY, toMyGuiButton( evt.button ) );
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
	}else if( evt.button == TumbuInput::PAD_MENU ){
		togglePauseMenu();
	}
	return true;
}
//-------------------------------------------------------------------------------------
