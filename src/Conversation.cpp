#include "Conversation.h"
//-------------------------------------------------------------------------------------
Conversation::Conversation(std::string characterName, std::string image, std::string message){
	mCharacterName = characterName;
	mImage = image;
	mMessage = message;

	showConversationOnTop = false;

	setType( Dialog::CONVERSATION );
}
//-------------------------------------------------------------------------------------
Conversation::~Conversation(void){
}
//-------------------------------------------------------------------------------------
void Conversation::update( const Ogre::Real time ){
}
//-------------------------------------------------------------------------------------
void Conversation::setGUI(
		CEGUI::Window* _mainConversation, CEGUI::Window* _conversationTop, CEGUI::Window* _conversationTopImage,
		CEGUI::Window* _conversationTopMessage, CEGUI::Window* _conversationTopContinueButton,
		CEGUI::Window* _conversationBottom, CEGUI::Window* _conversationBottomImage, CEGUI::Window* _conversationBottomMessage,
		CEGUI::Window* _conversationBottomContinueButton){

	mainConversation = _mainConversation;
	conversationTop = _conversationTop;
	conversationTopImage = _conversationTopImage;
	conversationTopMessage = _conversationTopMessage;
	conversationTopContinueButton = _conversationTopContinueButton;
	conversationBottom = _conversationBottom;
	conversationBottomImage = _conversationBottomImage;
	conversationBottomMessage = _conversationBottomMessage;
	conversationBottomContinueButton = _conversationBottomContinueButton;
}
//-------------------------------------------------------------------------------------
void Conversation::showDialog(){
	mainConversation->setVisible(true);
	if( showConversationOnTop ){
		conversationTop->setText( mCharacterName );
		conversationTopImage->setProperty("Image", "set:RobotFaces image:" + mImage);
		conversationTopMessage->setText( mMessage );

		conversationTop->setVisible(true);

		conversationTopContinueButton->setEnabled(true);
		conversationBottomContinueButton->setEnabled(false);
	}else{
		conversationBottom->setText( mCharacterName );
		conversationBottomImage->setProperty("Image", "set:RobotFaces image:" + mImage);
		conversationBottomMessage->setText( mMessage );
 
		conversationBottom->setVisible(true);

		conversationTopContinueButton->setEnabled(false);
		conversationBottomContinueButton->setEnabled(true);
	}
}
//-------------------------------------------------------------------------------------