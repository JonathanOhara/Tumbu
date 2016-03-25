#include "Confirm.h"
//-------------------------------------------------------------------------------------
Confirm::Confirm( std::string _title, std::string _message ){
	title = _title;
	message = _message;

	setType( Dialog::CONFIRM );
}
//-------------------------------------------------------------------------------------
Confirm::~Confirm(void){

}
//-------------------------------------------------------------------------------------
void Confirm::update( const Ogre::Real time ){

}
//-------------------------------------------------------------------------------------
void Confirm::setGUI( CEGUI::Window* _mainDialogConfirm, CEGUI::Window* _dialogConfirmText, CEGUI::Window* _dialogConfirmOkButton, CEGUI::Window* _dialogConfirmCancelButton ){
	mainDialogConfirm = _mainDialogConfirm ;
	dialogConfirmText = _dialogConfirmText;
	dialogConfirmOkButton = _dialogConfirmOkButton;
	dialogConfirmCancelButton = _dialogConfirmCancelButton;
}
//-------------------------------------------------------------------------------------
void Confirm::showDialog(){
	dialogConfirmText->setText( message );
	mainDialogConfirm->setText( title );
	mainDialogConfirm->setVisible(true);
	mainDialogConfirm->setAlwaysOnTop(true);
}
//-------------------------------------------------------------------------------------