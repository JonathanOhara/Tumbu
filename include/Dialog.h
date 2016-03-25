#ifndef __Dialog_h_
#define __Dialog_h_

#include <Ogre.h>
#include <OIS.h>
#include <CEGUI.h>
#include <RendererModules/Ogre/CEGUIOgreRenderer.h>
#include <iostream>

using namespace std;

class Dialog {
public:
	Dialog();
	virtual ~Dialog(void);

	virtual void update(const Ogre::Real time) = 0;
	virtual void showDialog() = 0;

	enum DialogType{ ALERT, CONFIRM, CONVERSATION, SHOW_PART };

	DialogType getType();
	void setType( DialogType _type );

	bool isAnswered();
	void setAnswered( bool _answered );
	
	bool isOk();
	void setOk( bool _ok ); 

	bool isCancel();
	void setCancel( bool _cancel );

protected:
private:

	DialogType type;
	
	bool 
		answered,
		ok,
		cancel;
};

#endif // #ifndef __Dialog_h_