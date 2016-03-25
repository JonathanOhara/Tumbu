#ifndef __LogManager_h_
#define __LogManager_h_

#include <Ogre.h>
#include <OIS.h>
#include <math.h>

class GUI;

#include "Log.h"

using namespace std;

class LogManager{
public:
	LogManager();
	LogManager(GUI *_gui);
	virtual ~LogManager(void);

	void reset();

	void update(const Ogre::Real time);

	void addLog( Log::LogType _logType, std::string message, float duration );
protected:

private:
	void hideAllLogs();
	void clearLog( int order );
	void fillLog( Log *log, int order );

	std::list<Log*> logList;
	
	GUI *gui;

	/** Para os iterators */
	Log* _log;
	std::list<Log*>::iterator itLogList;
	std::list<Log*>::iterator itLogListEnd;
};

#endif // #ifndef __LogManager_h_
