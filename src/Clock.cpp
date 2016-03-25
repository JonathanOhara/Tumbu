#include "Clock.h"
#include "Tumbu.h"
#include "GUI.h"
Clock* Clock::instance = NULL;
//-------------------------------------------------------------------------------------
Clock::Clock( int _minutes ){
	hours = (_minutes / 60) % 1440;
	
	timeMultiplier = TUMBU::getInstance()->getTimeMultiplier();

	if( hours < 6 ){
		dayType = TumbuEnums::NIGHT;
	}else if( hours < 19 ){
		dayType = TumbuEnums::MORNING;
	}else{
		dayType = TumbuEnums::NIGHT;
	}
}
//-------------------------------------------------------------------------------------
Clock::~Clock(void){
}
//-------------------------------------------------------------------------------------
Clock* Clock::getInstance(){
	//NOTE: SkyX start at 14 hours
	if( instance == NULL){
		instance = new Clock( 780 );
	}
	return instance;
}
//-------------------------------------------------------------------------------------
int Clock::getMinutes(){
	return (int) hours * 60;
}
//-------------------------------------------------------------------------------------
void Clock::setMinutes( int _minutes ){
	hours = _minutes / 60;
}
//-------------------------------------------------------------------------------------
std::string Clock::getClockFormated(){
	int minutes = hours * 60 ;
	int numberOfHours = minutes / 60,
		numberOfMinutes = minutes % 60;

	Ogre::String tempo("Time: ");

	if(numberOfHours < 10){
		tempo.append("0");
	}

	tempo.append( Ogre::StringConverter::toString(numberOfHours) ).append(":");
	if(numberOfMinutes < 10){
		tempo.append("0");
	}
	tempo.append( Ogre::StringConverter::toString(numberOfMinutes) );

	return tempo.c_str();
}
//-------------------------------------------------------------------------------------
TumbuEnums::DayType Clock::getDayType(){
	return dayType;
}
//-------------------------------------------------------------------------------------
bool Clock::frameRenderingQueued(const Ogre::FrameEvent &evt){
	if( TUMBU::getInstance()->isPlaying() ){
		hours += (evt.timeSinceLastFrame * timeMultiplier);

		if( (int) hours == 6 ){
			dayType = TumbuEnums::MORNING;
		}else if( (int) hours == 19 ){
			dayType = TumbuEnums::NIGHT;
		}else if ( (int)  hours >= 24){
			hours -= 24;
		}
	}
	return true;
}
//-------------------------------------------------------------------------------------