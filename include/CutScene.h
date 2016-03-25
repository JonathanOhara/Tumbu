#ifndef __CutScene_h_
#define __CutScene_h_

#include <Ogre.h>
#include <OIS.h>

#include <iostream>

using namespace std;

class CutScene: public Ogre::FrameListener{
public:
	CutScene(void);
    virtual ~CutScene(void);
	
	virtual bool frameRenderingQueued(const Ogre::FrameEvent &evt);
protected:
	Ogre::SceneManager *sceneManager;
private:
};

#endif // #ifndef __CutScene_h_