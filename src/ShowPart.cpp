#include "ShowPart.h"
//-------------------------------------------------------------------------------------
ShowPart::ShowPart( std::string _title, std::string _message, Part *_part, Ogre::Root* _root, CEGUI::OgreRenderer *_cegui ){
	title = _title;
	message = _message;
	part = _part;
	root = _root;
	cegui = _cegui;

	setType( Dialog::SHOW_PART );
}
//-------------------------------------------------------------------------------------
ShowPart::~ShowPart(void){
	auxSceneManager->destroyCamera(auxCamera);
			
	renderTexture->removeAllViewports();
	renderTexture->removeAllListeners();
	root->getTextureManager()->remove("RTT");
			
	root->destroySceneManager(auxSceneManager);

	CEGUI::ImagesetManager::getSingleton().destroy( CEGUI::ImagesetManager::getSingleton().get("RTTImageset") );
}
//-------------------------------------------------------------------------------------
void ShowPart::update( const Ogre::Real time ){
	no->yaw( Ogre::Radian(0.5f * time) );
}
//-------------------------------------------------------------------------------------
void ShowPart::setGUI( CEGUI::Window* _mainDialogShowPart, CEGUI::Window* _dialogShowPartText, CEGUI::Window* _dialogShowPartImage, CEGUI::Window* _dialogShowPartOkButton ){
	mainDialogShowPart = _mainDialogShowPart;
	dialogShowPartText = _dialogShowPartText;
	dialogShowPartImage = _dialogShowPartImage;
	dialogShowPartOkButton = _dialogShowPartOkButton;
}
//-------------------------------------------------------------------------------------
void ShowPart::showDialog(){
	Ogre::String text(message);

	auxSceneManager = root->createSceneManager(Ogre::ST_GENERIC, "showPartSceneManager");
	no = auxSceneManager->getRootSceneNode()->createChildSceneNode("ShowPartSceneNode");
	entity = auxSceneManager->createEntity("showPartEntity", part->meshName);
	no->attachObject(entity);

	no->setPosition( Ogre::Vector3::ZERO );

	Ogre::Light* luz = auxSceneManager->createLight("luz_esc");
	luz->setPosition( Ogre::Vector3( 0, 0, 5000 ) );

	auxTexture = root->getTextureManager()->createManual(
		"RTT_ShowPart",
		Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		Ogre::TEX_TYPE_2D,
		512,
		512,
		0,
		Ogre::PF_R8G8B8,
		Ogre::TU_RENDERTARGET);
	renderTexture = auxTexture->getBuffer()->getRenderTarget();
 
	auxCamera = auxSceneManager->createCamera("RTTCam");
	auxCamera->setPosition(0, 0, 1.5f);
	auxCamera->lookAt(0, 0, 0);
	auxCamera->setNearClipDistance(0.001f);
	auxCamera->setFarClipDistance(1000);

	v = renderTexture->addViewport(auxCamera);
	v->setOverlaysEnabled(false);
	v->setClearEveryFrame(true);

	v->setBackgroundColour(Ogre::ColourValue::Black);

	CEGUI::Texture &guiTex = cegui->createTexture(auxTexture);
	CEGUI::Imageset &imageSet = CEGUI::ImagesetManager::getSingleton().create("RTTImageset", guiTex);

	imageSet.defineImage("RTTImage",
							CEGUI::Point(0.0f, 0.0f),
							CEGUI::Size(guiTex.getSize().d_width, guiTex.getSize().d_height),
							CEGUI::Point(0.0f, 0.0f));

	dialogShowPartImage->setProperty("Image", CEGUI::PropertyHelper::imageToString( &imageSet.getImage("RTTImage") ));

	part->loadParameters();

	mainDialogShowPart->setText( title );

	text.append( " \n \nHp: " ).append( Ogre::StringConverter::toString((int)part->hp ) );
	text.append( "\nAp: " ).append( Ogre::StringConverter::toString((int)part->ap ) );
	text.append( "\nAttack: " ).append( Ogre::StringConverter::toString((int)part->attack ) );
	text.append( "\nDefense: " ).append( Ogre::StringConverter::toString((int)part->defense ) );
	text.append( "\nVelocity: " ).append( Ogre::StringConverter::toString((int)part->velocity ) );

	dialogShowPartText->setText( text );

	dialogShowPartImage->setVisible(true);
	dialogShowPartImage->setAlwaysOnTop(true);

	mainDialogShowPart->setVisible(true);
	mainDialogShowPart->setAlwaysOnTop(true);	
}
//-------------------------------------------------------------------------------------