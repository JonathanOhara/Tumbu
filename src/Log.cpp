#include "Log.h"
//-------------------------------------------------------------------------------------
Log::Log( LogType _logType, std::string _message, float _duration ){
	logType		= _logType;
	message		= _message;
	duration	= _duration;

	logStatus = LIVE;

	switch ( logType ){
	case NORMAL:
		//Black
		colorString = "tl:FF000000 tr:FF000000 bl:FF000000 br:FF000000";
		break;
	case WARNING:
		//Yellow
		colorString = "tl:FFFFFF00 tr:FFFFFF00 bl:FFFFFF00 br:FFFFFF00";
		break;
	case FAILED:
		//Red
		colorString = "tl:FFFF0000 tr:FFFF0000 bl:FFFF0000 br:FFFF0000";
		break;
	case LEVEL_UP:
		//Blue
		colorString = "tl:FF0000FF tr:FF0000FF bl:FF0000FF br:FF0000FF";
		break;
	case GREEN:
		//Yellow
		colorString = "tl:FF00FF00 tr:FF00FF00 bl:FF00FF00 br:FF00FF00";
		break;
	}
}
//-------------------------------------------------------------------------------------
Log::~Log(void){
}
//-------------------------------------------------------------------------------------
void Log::update(const Ogre::Real time){
	if( logStatus == LIVE){
		duration -= time;

		if(duration <= 0){
			logStatus = TO_DELETE;
		}
	}
}
//-------------------------------------------------------------------------------------
Log::LogStatus Log::getLogStatus(){
	return logStatus;
}
//-------------------------------------------------------------------------------------
void Log::setLogStatus( LogStatus _logStatus ){
	logStatus = _logStatus;
}
//-------------------------------------------------------------------------------------
Log::LogType Log::getLogType(){
	return logType;
}
//-------------------------------------------------------------------------------------
void Log::setLogType( LogType _logType ){
	logType = _logType;
}
//-------------------------------------------------------------------------------------
int Log::getOrder(){
	return order;
}
//-------------------------------------------------------------------------------------
void Log::setOrder( int _order ){
	order = _order;
}
//-------------------------------------------------------------------------------------
std::string Log::getMessage(){
	return message;
}
//-------------------------------------------------------------------------------------
void Log::setMessage( std::string _message ){
	message = _message;
}
//-------------------------------------------------------------------------------------
std::string Log::getColorString(){
	return colorString;
}
//-------------------------------------------------------------------------------------
void Log::setColorString( std::string _colorString ){
	colorString = _colorString;
}
//-------------------------------------------------------------------------------------