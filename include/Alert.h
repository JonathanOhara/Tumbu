#ifndef __Alert_h_
#define __Alert_h_

#include <Ogre.h>
#include <OIS.h>
#include "Dialog.h"

class Alert: public Dialog {
public:
	Alert( std::string _title, std::string _message );
	virtual ~Alert(void);
	
	void update(const Ogre::Real time);

	void setGUI( CEGUI::Window* _mainDialogAlert, CEGUI::Window* _dialogAlertText, CEGUI::Window* _dialogAlertOkButton );
	void showDialog();
protected:
private:

	std::string title;
	std::string message;

	CEGUI::Window
		*mainDialogAlert,
		*dialogAlertText,
		*dialogAlertOkButton;
};

#endif // #ifndef __Alert_h_