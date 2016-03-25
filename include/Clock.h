#ifndef __Clock_h_
#define __Clock_h_

#include <Ogre.h>
#include <OIS.h>
#include "GUI.h"
#include "Enums.h"
#include "TUMBU.h"

class TUMBU;

class Clock: public Ogre::FrameListener{
public:
	Clock( int _minutes );
	virtual ~Clock(void);
	static Clock* getInstance(void);

	int getMinutes();
	void setMinutes( int _minutes );

	std::string getClockFormated();
	TumbuEnums::DayType getDayType();
	
	bool frameRenderingQueued(const Ogre::FrameEvent &evt);
protected:

private:
	Ogre::Real 
		hours,
		timeMultiplier;

	TumbuEnums::DayType dayType;

	static Clock* instance;
};

#endif // #ifndef __Clock_h_