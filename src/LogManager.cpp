#include "LogManager.h"
#include "GUI.h"
//-------------------------------------------------------------------------------------
LogManager::LogManager(GUI *_gui){
	gui = _gui;
}
//-------------------------------------------------------------------------------------
LogManager::LogManager(){
	gui = GUI::getInstance();
}
//-------------------------------------------------------------------------------------
LogManager::~LogManager(void){
	itLogList = logList.begin();
	itLogListEnd = logList.end();
	while ( itLogList != itLogListEnd ){
		_log = (*itLogList);

		itLogList = logList.erase( itLogList );
		delete _log;
	}

	logList.clear();
}
//-------------------------------------------------------------------------------------
void LogManager::reset(){
	itLogList = logList.begin();
	itLogListEnd = logList.end();
	while ( itLogList != itLogListEnd ){
		_log = (*itLogList);

		itLogList = logList.erase( itLogList );
		delete _log;
	}

	logList.clear();
}
//-------------------------------------------------------------------------------------
void LogManager::update(const Ogre::Real time){
	if( logList.size() > 0 ){
		int count = 0;

		itLogList = logList.begin();
		itLogListEnd = logList.end();
		while ( itLogList != itLogListEnd ){
			_log = (*itLogList);
			if( _log->getLogStatus() == Log::TO_DELETE ){
				itLogList = logList.erase( itLogList );

				clearLog( _log->getOrder() );

				delete _log;
			}else{
				_log->update(time);
					
				if( _log->getOrder() != count ){
					
					clearLog( _log->getOrder() );
					_log->setOrder( count );
				}
					
				fillLog( _log, count );
				count++;
				itLogList++;
			}
				
		}
		gui->rootLog->setVisible( true );
	}else{
		hideAllLogs();
	}
}
//-------------------------------------------------------------------------------------
void LogManager::addLog( Log::LogType _logType, std::string message, float duration ){
	Log* log = new Log( _logType, message, duration );

	if( logList.size() == 4 ){
		std::list<Log*>::iterator i = logList.begin();
		Log* _log = *i;
		logList.erase(i);
		delete _log;
	}
	logList.push_back(log);
}
//-------------------------------------------------------------------------------------
void LogManager::hideAllLogs(  ){
	gui->rootLog->setVisible( false );
	gui->log1->setVisible( false );
	gui->log2->setVisible( false );
	gui->log3->setVisible( false );
	gui->log4->setVisible( false );
}
//-------------------------------------------------------------------------------------
void LogManager::clearLog( int order  ){
	switch( order ){
	case 0:
		gui->log1->setText( "" );
		gui->log1->setVisible( false );
		break;
	case 1:
		gui->log2->setText( "" );
		gui->log2->setVisible( false );
		break;
	case 2:
		gui->log3->setText( "" );
		gui->log3->setVisible( false );
		break;
	case 3: 
		gui->log4->setText( "" );
		gui->log4->setVisible( false );
		break;
	}
}
//-------------------------------------------------------------------------------------
void LogManager::fillLog( Log *log , int order ){
	switch( order ){
	case 0:
		gui->log1->setText( log->getMessage() );
		gui->log1->setProperty("TextColours", log->getColorString() );
		gui->log1->setVisible( true );
		break;
	case 1:
		gui->log2->setText( log->getMessage() );
		gui->log2->setProperty("TextColours", log->getColorString() );
		gui->log2->setVisible( true );
		break;
	case 2:
		gui->log3->setText( log->getMessage() );
		gui->log3->setProperty("TextColours", log->getColorString() );
		gui->log3->setVisible( true );
		break;
	case 3: 
		gui->log4->setText( log->getMessage() );
		gui->log4->setProperty("TextColours", log->getColorString() );
		gui->log4->setVisible( true );
		break;
	}
}
//-------------------------------------------------------------------------------------