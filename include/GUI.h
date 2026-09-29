#ifndef __GUI_h_
#define __GUI_h_

#include <Ogre.h>
#include <queue>
#include <string>

#include "Input.h"
#include "Log.h"

class Demo;
class Part;
class Character;
class CharacterEnemy;

/**
 * Game user interface: start menu and options, loading screen, battle HUD (HP/AP bars), ESC menu
 * (status / inventory / skills / help), conversations and a queue of dialogs, an on-screen message log and
 * floating damage numbers.
 *
 * Dialogs are queued (addAlert/addConfirm/addConversation/addShowPart) and shown one at a time by
 * showNextDialog(); while a dialog is open the game state is IN_DIALOG. Dialog buttons can trigger a GUI
 * action (a `void GUI::action()` member such as gameOverOk or startTutorialMode).
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

	void pushDialog( const DialogEntry &dialog );
	void answerDialog( bool ok );

	Ogre::RenderWindow *mWindow;
	Ogre::Root *mRoot;
	Character *hero;
	CharacterEnemy *enemy;
	std::queue<DialogEntry> dialogList;
	DialogEntry activeDialog;
	bool hasActiveDialog;
	bool waitingUserContinue;
	bool startMenuVisible;
	bool optionsOpen;

	static GUI* instance;
};

#endif // #ifndef __GUI_h_
