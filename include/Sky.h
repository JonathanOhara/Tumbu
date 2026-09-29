#ifndef __Sky_h_
#define __Sky_h_

#include <Ogre.h>
#include <GUI.h>

#include "Clock.h"
#include "TUMBU.h"


class Clock;
namespace Caelum{ class CaelumSystem; }

class Sky: public Ogre::FrameListener{
public:
	Sky( Ogre::SceneManager* sceneMgr );
	virtual ~Sky(void);
	static Sky* getInstance(void);

	void setClock( Clock* _clock );
	void skyLowQuality();
	void skyLowQualityMorning();
	void skyLowQualityNight();
	void skyHighQuality(Ogre::Camera* camera);
	void updateCaelumTime(void);
	
	bool frameRenderingQueued(const Ogre::FrameEvent &evt);

protected:

private:
	TumbuEnums::DayType dayType;
	Ogre::Light* light;
	Ogre::SceneManager* mSceneMgr;

	Clock* clock;

	Ogre::Real 
		updateTime,
		timeMultiplier;

	int quality;

	/// Day/night sky for the "High" quality (Direct3D 11 only: Caelum has no GLSL shaders).
	Caelum::CaelumSystem* caelum;

	static Sky* instance;
};

#endif // #ifndef __Sky_h_