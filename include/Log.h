#ifndef __Log_h_
#define __Log_h_

#include <Ogre.h>
#include <OIS.h>
#include <math.h>

using namespace std;

class Log{
public:
	enum LogStatus { LIVE, TO_DELETE };
	enum LogType { NORMAL, WARNING, FAILED, LEVEL_UP, GREEN };

	Log( LogType _logType, std::string _message, float _duration );
	virtual ~Log(void);

	void update(const Ogre::Real time);

	LogStatus getLogStatus();
	void setLogStatus( LogStatus _logStatus );

	LogType getLogType();
	void setLogType( LogType _logType );

	int getOrder();
	void setOrder( int _order );

	std::string getMessage();
	void setMessage( std::string _message );

	std::string getColorString();
	void setColorString( std::string _colorString );
protected:

private:
	std::string 
		message,
		colorString;

	int 
		priority,
		order;

	float 
		duration;

	LogStatus logStatus;
	LogType logType;
};

#endif // #ifndef __Log_h_