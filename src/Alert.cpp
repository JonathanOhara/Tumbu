#include "Alert.h"
//-------------------------------------------------------------------------------------
Alert::Alert( std::string _title, std::string _message ){
	title = _title;
	message = _message;

	setType( Dialog::ALERT );
}
//-------------------------------------------------------------------------------------
Alert::~Alert(void){
}
//-------------------------------------------------------------------------------------
void Alert::update( const Ogre::Real time ){

}
//-------------------------------------------------------------------------------------
void Alert::setGUI( CEGUI::Window* _mainDialogAlert, CEGUI::Window* _dialogAlertText, CEGUI::Window* _dialogAlertOkButton ){
	mainDialogAlert = _mainDialogAlert;
	dialogAlertText = _dialogAlertText;
	dialogAlertOkButton = _dialogAlertOkButton;
}
//-------------------------------------------------------------------------------------
void Alert::showDialog(){
	dialogAlertText->setText( message );
	mainDialogAlert->setText( title );
	
	mainDialogAlert->setVisible(true);
	mainDialogAlert->setAlwaysOnTop(true);
}
//-------------------------------------------------------------------------------------