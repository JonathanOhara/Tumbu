#include "StartScreen.h"
//-------------------------------------------------------------------------------------
StartScreen::StartScreen(Ogre::SceneManager* mSceneMgr){
	sceneManager = mSceneMgr;
	timeCount = 0;
	// Create background material
	Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().create("Background_Shyds", "General");
	material->getTechnique(0)->getPass(0)->createTextureUnitState("shyds logo.png");
	material->getTechnique(0)->getPass(0)->setDepthCheckEnabled(false);
	material->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
	material->getTechnique(0)->getPass(0)->setLightingEnabled(false);

	// Create background rectangle covering the whole screen
	rect = new Ogre::Rectangle2D(true);
	rect->setCorners(-1.0, 1.0, 1.0, -1.0);
	rect->setMaterial(material->getName());
 
	// Render the background before everything else
	rect->setRenderQueueGroup(Ogre::RENDER_QUEUE_OVERLAY);
 
	// Hacky, but we need to set the bounding box to something big
	// NOTE: If you are using Eihort (v1.4), please see the note below on setting the bounding box
	rect->setBoundingBox(Ogre::AxisAlignedBox(-100000.0 * Ogre::Vector3::UNIT_SCALE, 100000.0 * Ogre::Vector3::UNIT_SCALE));
 
	// Attach background to the scene
	startScreenNode = sceneManager->getRootSceneNode()->createChildSceneNode("Background");
	startScreenNode->attachObject(rect);
}
//-------------------------------------------------------------------------------------
StartScreen::~StartScreen(void){
	startScreenNode->removeAndDestroyAllChildren();
	sceneManager->destroySceneNode(startScreenNode);

	delete rect;
}
//-------------------------------------------------------------------------------------
bool StartScreen::frameRenderingQueued(const Ogre::FrameEvent &evt){
//	cout << "Start Screen " << endl;
	return true;
}
//-------------------------------------------------------------------------------------
void StartScreen::showStartScreenImage(){
	rect->getMaterial()->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("startScreen_04.jpg");
}
//-------------------------------------------------------------------------------------