#ifndef __StartScreen_h_
#define __StartScreen_h_

#include "CutScene.h"

class StartScreen: public CutScene{
public:
	StartScreen(Ogre::SceneManager* mSceneMgr);
	virtual ~StartScreen(void);

	bool frameRenderingQueued(const Ogre::FrameEvent& evt);

	void showStartScreenImage();
protected:
private:

	Ogre::Rectangle2D* rect;
	Ogre::SceneNode* startScreenNode;
	Ogre::Real timeCount;
};
#endif // #ifndef __StartScreen_h_