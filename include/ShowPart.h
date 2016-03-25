#ifndef __ShowPart_h_
#define __ShowPart_h_

#include <CEGUI.h>
#include <Ogre.h>
#include <OIS.h>
#include "Dialog.h"
#include "Part.h"

class ShowPart: public Dialog {
public:
	ShowPart( std::string _title, std::string _message, Part *_part, Ogre::Root* _root, CEGUI::OgreRenderer *_cegui );
	virtual ~ShowPart(void);

	void update(const Ogre::Real time);

	void setGUI( CEGUI::Window* _mainDialogShowPart, CEGUI::Window* _dialogShowPartText, CEGUI::Window* _dialogShowPartImage, CEGUI::Window* _dialogShowPartOkButton );
	void showDialog();
protected:
private:
	std::string title;
	std::string message;

	Ogre::Root *root;
	CEGUI::OgreRenderer *cegui;

	Ogre::SceneManager *auxSceneManager;
	Ogre::SceneNode *no;
	Ogre::Entity *entity;

	Ogre::Camera *auxCamera;
	Ogre::TexturePtr auxTexture;
	Ogre::RenderTexture *renderTexture;

	Part *part;

	Ogre::Viewport *v;

	CEGUI::Window
		*mainDialogShowPart,
		*dialogShowPartText,
		*dialogShowPartImage,
		*dialogShowPartOkButton;
};

#endif // #ifndef __ShowPart_h_