#ifndef __GUI_h_
#define __GUI_h_

#include <Ogre.h>
#include <OIS.h>
#include <CEGUI.h> 
#include <RendererModules/Ogre/CEGUIOgreRenderer.h>
#include <iostream>

#include "Dialog.h"
#include "Demo.h"
#include "Conversation.h"
#include "Alert.h"
#include "Confirm.h"
#include "ShowPart.h"
#include "Part.h"
#include "LogManager.h"
#include "SkillHitManager.h"

using namespace std;

class Demo;

class GUI: public OIS::KeyListener, public OIS::MouseListener, public OIS::JoyStickListener, public Ogre::FrameListener{
public:
	GUI(Ogre::RenderWindow* window, Ogre::Root *root);
	virtual ~GUI(void);
	static GUI* getInstance();
	
	void reset();

	void initOptions();
	void turnOptionOnOff();
	
	bool isOptionOn();
	
	bool mouseMoved(const OIS::MouseEvent &arg);
	bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);

	bool keyPressed(const OIS::KeyEvent &arg);
	bool keyReleased(const OIS::KeyEvent &arg);

    bool povMoved( const OIS::JoyStickEvent &e, int pov );
    bool axisMoved( const OIS::JoyStickEvent &e, int axis );
    bool sliderMoved( const OIS::JoyStickEvent &e, int sliderID );
    bool buttonPressed( const OIS::JoyStickEvent &e, int button );
    bool buttonReleased( const OIS::JoyStickEvent &e, int button );

	int getScreenWidth();
	int getScreenHeight();

	bool frameRenderingQueued(const Ogre::FrameEvent &evt);

	CEGUI::MouseButton convertOisCegui(const OIS::MouseButtonID id);
	
	void hideAllOptions();

	void hideOptionStatus();
	void hideOptionInventory();
	void hideOptionSkills();
	void hideOptionHelp();

	/** EVENTOS	*/
	bool isOptionStatusOn(const CEGUI::EventArgs &e);
	bool isOptionInventoryOn(const CEGUI::EventArgs &e);
	bool isOptionSkillsOn(const CEGUI::EventArgs &e);
	bool isOptionHelpOn(const CEGUI::EventArgs &e);
	bool dialogOk(const CEGUI::EventArgs &e);
	bool dialogCancel(const CEGUI::EventArgs &e);
	bool dialogShowPartOk(const CEGUI::EventArgs &e);
	bool continueTopConversation(const CEGUI::EventArgs &e);
	bool continueBottomConversation(const CEGUI::EventArgs &e);
	bool gameOverOk(const CEGUI::EventArgs &e);
	bool quit(const CEGUI::EventArgs &e);
	bool gameStart(const CEGUI::EventArgs &e);
	bool gameOptions(const CEGUI::EventArgs &e);
	bool gameExit(const CEGUI::EventArgs &e);
	bool gameOptionsOk(const CEGUI::EventArgs &e);
	bool gameOptionsDefault(const CEGUI::EventArgs &e);
	bool gameOptionsCancel(const CEGUI::EventArgs &e);
	bool changeRobotHead(const CEGUI::EventArgs &e);
	bool changeRobotBody(const CEGUI::EventArgs &e);
	bool changeRobotRightArm(const CEGUI::EventArgs &e);
	bool changeRobotLeftArm(const CEGUI::EventArgs &e);
	bool changeRobotLegs(const CEGUI::EventArgs &e);
	bool skillButtonClick(const CEGUI::EventArgs &e);
	bool startTutorialMode(const CEGUI::EventArgs &e);
	bool stopTutorialMode(const CEGUI::EventArgs &e);
	bool teste(const CEGUI::EventArgs &e);

	bool isWaitingUserToContinue();

	void updateLeftTopHpBar(float value, float percent);
	void updateLeftTopApBar(float value, float percent);

	void updateRightBottomHpBar(float value, float percent);
	void updateRightBottomApBar(float value, float percent);

	void startLoad(Ogre::String message);
	void stopLoad();

	void showBattleLayout();
	void hideBattleLayout();

	void setMouseCursorVisibility( bool visibility );

	void updateClockTime();

	/** DIALOG */
	Alert* addAlert( std::string title, std::string message );
	Alert* addAlert( std::string title, std::string message, bool (GUI::*function)(const CEGUI::EventArgs&) );
	Confirm* addConfirm( std::string title, std::string message );
	Confirm* addConfirm( std::string title, std::string message, bool (GUI::*okFunction)(const CEGUI::EventArgs&), bool (GUI::*cancelFunction)(const CEGUI::EventArgs&) );
	Conversation* addConversation( std::string characterName, std::string image, std::string message );
	ShowPart* addShowPart( std::string title, std::string message, Part *part );
	//ShowPart* addShowPart( std::string title, std::string message, Part *part, bool (GUI::*okFunction) );

	void showNextDialogByKeyBoard(bool ok);
	void showNextDialog(bool ok);

	void addLog( Log::LogType _logType, std::string message, float duration );
	void addSkillHit( Ogre::Vector3 position, Ogre::String message );

	void showStartMenu();
	void hideStartMenu();

	void showStartOptionsMenu();
	void hideStartOptionsMenu();
	void saveStartOptionsMenu();
	void loadStartOptionsMenu();
	void defaultStartOptionsMenu();

	void builCharacterPreview();
	void unbuildCharacterPreview();

	void setHero(Character* _hero);
	void setEnemy(CharacterEnemy* _enemy);

	/****** LOG ******/
	CEGUI::Window
		*rootWindow,
		*rootLog,
		*log1,
		*log2,
		*log3,
		*log4;

protected:

private:
	void loadLoadingLayout();
	void loadBattleLayout();
	void loadConversationLayout();
	void loadDialogLayout();
	void loadMenuLayout();
	void loadStartScreen();

	bool mouseRunning;

	int mouseRunningX,
		mouseRunningY;

	Character		*hero;
	CharacterEnemy	*enemy;

	Ogre::Real timeSinceLastFrame;

	Demo *demo;

	LogManager* logManager;
	SkillHitManager* skillHitManager;

	Dialog* lastActiveDialog;

	Ogre::Root *mRoot;
	Ogre::RenderWindow *mWindow;
	
	CEGUI::OgreRenderer *cegui;
	CEGUI::DefaultLogger *defaultLogger;

	Ogre::SceneManager *auxSceneMgr;
	Ogre::SceneNode 
		*auxRobotNode,
		*auxHeadNode,
		*auxBodyNode,
		*auxRightArmNode,
		*auxLeftArmNode,
		*auxLegsNode;
	Part 
		*headPart,
		*bodyPart,
		*rightArmPart,
		*leftArmPart,
		*legsPart;
	Ogre::Entity *auxEntity;
	Ogre::Camera *auxCamera;
	Ogre::TexturePtr auxTexture;
	Ogre::RenderTexture *renderTexture;
	bool auxSceneMgrActive;

	CEGUI::Window 
		/******** LOADING ********/
		*loadingLayout,
		*mainLoading,
		*loadingText,
		/******* MENU - ESC *******/
		*menuLayout,
		*options,
		*optionsStatusButton,
		*optionsInventoryButton,
		*optionsSkillsButton,
		*optionsHelpButton,
		*optionsQuitButton,
		*clock,
		*optionsStatusWindow,
		*optionsInventoryWindow,
		*optionsSkillsWindow,
		*optionsHelpWindow,
		/** STATUS MENU */
		*optionStatusHp,
		*optionStatusAp,
		*optionStatusAttack,
		*optionStatusDeffense,
		*optionStatusVelocity,
		*optionInventoryRenderToTexture,
		/** SKILLS MENU */
		*optionSkillsMovesListLabel,
		*optionSkillsStatesValue,
		/****** BATTLE ******/
		*battleLayout,
		*leftTop,
		*leftTopHpValue,
		*leftTopApValue,
		*rightBottomHpValue,
		*rightBottomApValue,
		*rightBottom,
		/** Conersation */
		*conversationLayout,
		*mainConversation,
		*conversationTop,
		*conversationTopImage,
		*conversationTopMessage,
		*conversationTopContinueButton,
		*conversationBottom,
		*conversationBottomImage,
		*conversationBottomMessage,
		*conversationBottomContinueButton,
		/** Dialog */
		*mainDialog,
		*mainDialogAlert,
		*dialogAlertText,
		*dialogAlertOkButton,
		*mainDialogConfirm,
		*dialogConfirmText,
		*dialogConfirmOkButton,
		*dialogConfirmCancelButton,
		*mainDialogShowPart,
		*dialogShowPartText,
		*dialogShowPartImage,
		*dialogShowPartOkButton;

	CEGUI::ProgressBar
		/****** BATTLE ******/
		*mainHpProgressBar,
		*mainApProgressBar,
		*enemyHpProgressBar,
		*enemyApProgressBar;

	CEGUI::String lastConversationName;

	bool lastConversationTop,
		lastDialogConversation,
		waitingUserContinue;

	std::queue<Dialog*> dialogList;
	
	CEGUI::Combobox	
		/** Inventory MENU */
		*optionInventoryHeadCombo,
		*optionInventoryBodyCombo,
		*optionInventoryRightArmCombo,
		*optionInventoryLeftArmCombo,
		*optionInventoryLegsCombo,
		/** StartScreen Options */
		*startScreenOptionSkyCombo,
		*startScreenOptionShadowsCombo;

	/** START SCREEN */
	CEGUI::Window
		*startScreenLayout,
		*startScreenMenu,
		*startScreenOptionMenu,
		*startScreenCampaignButton,
		*startScreenOptionsButton,
		*startScreenExitButton,
		*startScreenOptionOkButton,
		*startScreenOptionDefaultButton,
		*startScreenOptionCancelButton;


	static GUI* instance;
};

#endif // #ifndef __GUI_h_