#ifndef __Conversation_h_
#define __Conversation_h_

#include "Dialog.h"

class Conversation: public Dialog {
public:
	Conversation(std::string characterName, std::string image, std::string message);
	virtual ~Conversation(void);

	void update(const Ogre::Real time);

	void setGUI(CEGUI::Window* _mainConversation, CEGUI::Window* _conversationTop, CEGUI::Window* _conversationTopImage,
		CEGUI::Window* _conversationTopMessage, CEGUI::Window* _conversationTopContinueButton,
		CEGUI::Window* _conversationBottom, CEGUI::Window* _conversationBottomImage, CEGUI::Window* _conversationBottomMessage,
		CEGUI::Window* _conversationBottomContinueButton);

	void showDialog();

	std::string mCharacterName;
	std::string mImage;
	std::string mMessage;

	bool showConversationOnTop;

protected:
private:
	CEGUI::Window
		*mainConversation,
		*conversationTop,
		*conversationTopImage,
		*conversationTopMessage,
		*conversationTopContinueButton,
		*conversationBottom,
		*conversationBottomImage,
		*conversationBottomMessage,
		*conversationBottomContinueButton;
};

#endif // #ifndef __Conversation_h_