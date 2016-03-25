#include "SkillHit.h"
#include "GUI.h"
#include "TUMBU.h"
//-------------------------------------------------------------------------------------
SkillHit::SkillHit( Ogre::Vector3 _position, Ogre::String _message ){
	Ogre::Camera* camera = TUMBU::getInstance()->getDemo()->getCamera()->mCamera;
	Ogre::Vector3 hcsPosition = camera->getProjectionMatrix() * camera->getViewMatrix() * _position; 

	message = _message;

	double x = ((hcsPosition.x * 0.5f) + 0.5f);
	double y = 1.0f - ((hcsPosition.y * 0.5f) + 0.5f);

	top  = y;
	left = x + 0.1f;
	height = 0.1f;
	width  = 0.5f;

	setSkillHitStatus( INITIALIZED );
}
//-------------------------------------------------------------------------------------
SkillHit::~SkillHit(void){
	switch( getSkillHitStatus() ){
	case TO_DELETE:
		GUI::getInstance()->rootWindow->removeChildWindow( label );
		break;
	case LIVE:
		GUI::getInstance()->rootWindow->removeChildWindow( label );
		break;
	}
}
//-------------------------------------------------------------------------------------
SkillHit::SkillHitStatus SkillHit::getSkillHitStatus(){
	return skillHitStatus;
}
//-------------------------------------------------------------------------------------
void SkillHit::setSkillHitStatus( SkillHitStatus _skillHitStatus ){
	skillHitStatus = _skillHitStatus;
}
//-------------------------------------------------------------------------------------
void SkillHit::initializeLabel(){
	CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();
	label = wmgr.createWindow("TaharezLook/StaticText");

	label->setProperty("BackgroundEnabled", "false");
	label->setProperty("FrameEnabled", "false");
	label->setProperty("TextColours", "tl:FFFF0000 tr:FFFF0000 bl:FFFF0000 br:FFFF0000");
	label->setProperty("Font", "DejaVuSans-32");

	label->setPosition( CEGUI::UVector2( CEGUI::UDim( left, 0) , CEGUI::UDim( top, 0 ) ) );
	label->setSize( CEGUI::UVector2( CEGUI::UDim( width , 0) , CEGUI::UDim( height, 0 ) ) );
		
}
//-------------------------------------------------------------------------------------
void SkillHit::moveUp(const Ogre::Real time){
	
	switch( skillHitStatus ){
	case LIVE:
		top -= time * 0.075f;
		label->setPosition( CEGUI::UVector2( CEGUI::UDim( left, 0 ) , CEGUI::UDim( top, 0 ) ) );

		if ( liveTime <= 0 ){
			setSkillHitStatus( TO_DELETE );
		}
	
		liveTime -= time;
		break;
	case INITIALIZED:
		initializeLabel();

		setSkillHitStatus( CREATED );
		break;
	case CREATED:
		label->setText( message );

		GUI::getInstance()->rootWindow->addChildWindow( label );

		liveTime = 1.75f;

		setSkillHitStatus( LIVE );
		break;
	}

}
//-------------------------------------------------------------------------------------
