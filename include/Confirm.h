#ifndef __Confirm_h_
#define __Confirm_h_

#include <Ogre.h>
#include <OIS.h>
#include "Dialog.h"

class Confirm: public Dialog {
public:
	Confirm( std::string _title, std::string _message );
	virtual ~Confirm(void);
	
	void update(const Ogre::Real time);

	void setGUI( CEGUI::Window* _mainDialogConfirm, CEGUI::Window* _dialogConfirmText, CEGUI::Window* _dialogConfirmOkButton, CEGUI::Window* _dialogConfirmCancelButton);
	void showDialog();
protected:
private:
	std::string title;
	std::string message;

	CEGUI::Window
		*mainDialogConfirm,
		*dialogConfirmText,
		*dialogConfirmOkButton,
		*dialogConfirmCancelButton;
};

#endif // #ifndef __Confirm_h_