#ifndef __GUI_h_
#define __GUI_h_

#include <Ogre.h>
#include <list>
#include <queue>
#include <string>
#include <vector>

#include "Input.h"
#include "Log.h"

class Demo;
class Part;
class Character;
class CharacterEnemy;

namespace MyGUI{
	class Gui;
	class OgrePlatform;
	class Widget;
	class Window;
	class Button;
	class TextBox;
	class EditBox;
	class ImageBox;
	class ComboBox;
	class ProgressBar;
}

/**
 * Game user interface (MyGUI, BlackBlue theme; layouts in media/gui/*.layout).
 *
 * Screens: start menu + options, loading text, battle HUD (HP/AP bars, message log, floating damage
 * numbers), ESC pause menu (status / inventory with a rotating 3D preview / skills / help / quit),
 * conversations and a queue of dialogs.
 *
 * Dialogs are queued (addAlert/addConfirm/addConversation/addShowPart) and shown one at a time by
 * showNextDialog(); while a dialog is open the game state is IN_DIALOG. Dialog buttons can trigger a GUI
 * action (a `void GUI::action()` member such as gameOverOk or startTutorialMode).
 * Keyboard: SPACE/ENTER = OK, ESC = cancel / pause menu. Gamepad: A = OK, B = cancel, START = pause menu.
 */
class GUI: public OgreBites::InputListener, public Ogre::FrameListener{
public:
	typedef void (GUI::*Action)(void);

	GUI( Ogre::RenderWindow* window, Ogre::Root *root );
	virtual ~GUI(void);
	static GUI* getInstance(void);

	void reset(void);
	void initOptions(void);
	bool isOptionOn(void);
	int getScreenWidth(void);
	int getScreenHeight(void);

	/** DIALOGS */
	void addAlert( const std::string &title, const std::string &message, Action okAction = NULL );
	void addConfirm( const std::string &title, const std::string &message, Action okAction, Action cancelAction );
	void addConversation( const std::string &characterName, const std::string &image, const std::string &message );
	void addShowPart( const std::string &title, const std::string &message, Part *part );
	void showNextDialog( bool ok );
	bool isWaitingUserToContinue(void);

	/** HUD */
	void addLog( Log::LogType logType, const std::string &message, float duration );
	void addSkillHit( const Ogre::Vector3 &position, const Ogre::String &message );
	void startLoad( const Ogre::String &message );
	void stopLoad(void);
	void showBattleLayout(void);
	void hideBattleLayout(void);
	void showStartMenu(void);
	void hideStartMenu(void);
	void setHero( Character* hero );
	void setEnemy( CharacterEnemy* enemy );
	void setMouseCursorVisibility( bool visible );

	/** ACTIONS (menu buttons and dialog callbacks) */
	void gameStart(void);
	void gameExit(void);
	void gameOverOk(void);
	void dialogOk(void);
	void dialogCancel(void);
	void startTutorialMode(void);
	void stopTutorialMode(void);

	bool frameRenderingQueued( const Ogre::FrameEvent &evt );
	bool keyPressed( const OgreBites::KeyboardEvent &evt );
	bool keyReleased( const OgreBites::KeyboardEvent &evt );
	bool mouseMoved( const OgreBites::MouseMotionEvent &evt );
	bool mouseWheelRolled( const OgreBites::MouseWheelEvent &evt );
	bool mousePressed( const OgreBites::MouseButtonEvent &evt );
	bool mouseReleased( const OgreBites::MouseButtonEvent &evt );
	bool buttonPressed( const OgreBites::ButtonEvent &evt );

private:
	struct DialogEntry{
		enum Type { ALERT, CONFIRM, CONVERSATION, SHOW_PART };
		Type type;
		std::string title, message, image;
		Part *part;
		Action okAction, cancelAction;
	};
	struct LogEntry{
		std::string message;
		Log::LogType type;
		float remaining;
	};
	struct SkillHitEntry{
		MyGUI::TextBox *label;
		Ogre::Vector3 position;
		float age;
	};

	// setup
	void loadLayouts(void);
	void connectEvents(void);
	template<class T> T* widget( const std::string &name );

	// dialogs
	void pushDialog( const DialogEntry &dialog );
	void answerDialog( bool ok );
	void hideDialogWidgets(void);
	void showDialogWidgets( const DialogEntry &dialog );

	// pause menu (ESC)
	void togglePauseMenu(void);
	void showPauseTab( MyGUI::Widget *panel );
	void fillStatus(void);
	void fillInventory(void);
	void fillSkills(void);
	void showSkillDetails( const std::string &skillID );
	void changePart( int partType, size_t index );

	// start screen options
	void showOptions(void);
	void loadOptionsIntoWidgets(void);
	void applyOptionsFromWidgets(void);

	// HUD
	void updateHud(void);
	void updateLog( float time );
	void updateSkillHits( float time );

	// 3D preview (inventory robot, rewarded part)
	void createPreviewScene(void);
	void showPreviewRobot(void);
	void showPreviewPart( Part *part );
	void clearPreview(void);
	void setPreviewActive( bool active );

	// MyGUI event handlers
	void onPlayClick( MyGUI::Widget *sender );
	void onOptionsClick( MyGUI::Widget *sender );
	void onExitClick( MyGUI::Widget *sender );
	void onOptionsOk( MyGUI::Widget *sender );
	void onOptionsDefault( MyGUI::Widget *sender );
	void onOptionsCancel( MyGUI::Widget *sender );
	void onDialogOk( MyGUI::Widget *sender );
	void onDialogCancel( MyGUI::Widget *sender );
	void onPauseTab( MyGUI::Widget *sender );
	void onSkillClick( MyGUI::Widget *sender );
	void onPartComboAccept( MyGUI::ComboBox *sender, size_t index );

	Ogre::RenderWindow *mWindow;
	Ogre::Root *mRoot;
	MyGUI::OgrePlatform *mPlatform;
	MyGUI::Gui *mGui;
	Character *hero;
	CharacterEnemy *enemy;

	std::queue<DialogEntry> dialogList;
	DialogEntry activeDialog;
	bool hasActiveDialog;
	bool waitingUserContinue;
	bool startMenuVisible;
	bool lastConversationTop;
	std::string lastConversationName;

	std::list<LogEntry> logList;
	std::list<SkillHitEntry> skillHits;
	int mouseX, mouseY, mouseZ;

	// preview scene
	Ogre::SceneManager *previewSceneMgr;
	Ogre::SceneNode *previewRoot;
	Ogre::Camera *previewCamera;
	Ogre::TexturePtr previewTexture;
	std::vector<Part*> previewParts;
	Ogre::Entity *previewEntity;
	bool previewRotating;

	// widgets
	MyGUI::Widget *startMenu, *battleHud, *pauseMenu;
	MyGUI::Widget *statusPanel, *inventoryPanel, *skillsPanel, *helpPanel, *skillsList;
	MyGUI::Window *optionsWindow, *alertWindow, *confirmWindow, *showPartWindow, *conversationTop, *conversationBottom, *skillDetails;
	MyGUI::TextBox *loadingText, *clockText;
	MyGUI::ImageBox *loadingScreen;	// full-screen cover while a match loads
	MyGUI::TextBox *logLines[4];
	MyGUI::ProgressBar *heroHpBar, *heroApBar, *enemyHpBar, *enemyApBar;
	MyGUI::TextBox *heroHpValue, *heroApValue;
	MyGUI::ComboBox *skyCombo, *shadowsCombo, *frameLimitCombo, *antiAliasingCombo;
	MyGUI::ComboBox *partCombos[5];

	static GUI* instance;
};

#endif // #ifndef __GUI_h_
