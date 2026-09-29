#include "TUMBU.h"
#include <exception>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include <dbghelp.h>
#include <cstdio>
#include <csignal>
#pragma comment( lib, "dbghelp.lib" )

/**
 * Crash report: when the game crashes (access violation, uncaught exception, abort) the call stack is written
 * to ogre.log and to %USERPROFILE%\Tumbu\crash.log, with function names and source lines where PDBs exist
 * (always for RelWithDebInfo). Windows then carries on with its usual crash handling.
 */
static volatile LONG crashReported = 0;
//-------------------------------------------------------------------------------------
static void writeCrashReport( const Ogre::String &title, CONTEXT context ){
	if( InterlockedExchange( &crashReported, 1 ) != 0 ){
		return;	// one report per run
	}
	HANDLE process = GetCurrentProcess();
	HANDLE thread = GetCurrentThread();
	SymSetOptions( SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS );
	SymInitialize( process, NULL, TRUE );

	Ogre::String report = "CRASH: " + title + "\n";
	char text[512];
	STACKFRAME64 frame = {};
	frame.AddrPC.Offset = context.Rip;		frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = context.Rbp;	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Offset = context.Rsp;	frame.AddrStack.Mode = AddrModeFlat;

	for( int depth = 0; depth < 48; depth++ ){
		if( !StackWalk64( IMAGE_FILE_MACHINE_AMD64, process, thread, &frame, &context, NULL,
				SymFunctionTableAccess64, SymGetModuleBase64, NULL ) || frame.AddrPC.Offset == 0 ){
			break;
		}
		DWORD64 address = frame.AddrPC.Offset;
		char symbolBuffer[sizeof( SYMBOL_INFO ) + MAX_SYM_NAME];
		SYMBOL_INFO *symbol = (SYMBOL_INFO*) symbolBuffer;
		symbol->SizeOfStruct = sizeof( SYMBOL_INFO );
		symbol->MaxNameLen = MAX_SYM_NAME;
		DWORD64 symbolOffset = 0;
		const char *name = SymFromAddr( process, address, &symbolOffset, symbol ) ? symbol->Name : "?";

		IMAGEHLP_MODULE64 module = {};
		module.SizeOfStruct = sizeof( module );
		const char *moduleName = SymGetModuleInfo64( process, address, &module ) ? module.ModuleName : "?";

		IMAGEHLP_LINE64 line = {};
		line.SizeOfStruct = sizeof( line );
		DWORD lineOffset = 0;
		if( SymGetLineFromAddr64( process, address, &lineOffset, &line ) ){
			snprintf( text, sizeof( text ), "  #%02d %s!%s  %s:%lu", depth, moduleName, name, line.FileName, line.LineNumber );
		}else{
			snprintf( text, sizeof( text ), "  #%02d %s!%s +0x%llx", depth, moduleName, name, symbolOffset );
		}
		report += Ogre::String( text ) + "\n";
	}
	SymCleanup( process );

	if( Ogre::LogManager::getSingletonPtr() ){
		Ogre::LogManager::getSingleton().logError( report );
	}
	const char *home = getenv( "USERPROFILE" );
	FILE *file = fopen( ( Ogre::String( home ? home : "." ) + "/Tumbu/crash.log" ).c_str(), "w" );
	if( file ){
		fputs( report.c_str(), file );
		fclose( file );
	}
}
//-------------------------------------------------------------------------------------
static LONG WINAPI onCrash( EXCEPTION_POINTERS *info ){
	// Vectored handlers see every exception first (C++ exceptions included): only report real crashes.
	switch( info->ExceptionRecord->ExceptionCode ){
	case EXCEPTION_ACCESS_VIOLATION: case EXCEPTION_ILLEGAL_INSTRUCTION: case EXCEPTION_INT_DIVIDE_BY_ZERO:
	case EXCEPTION_STACK_OVERFLOW: case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: case EXCEPTION_IN_PAGE_ERROR:
		break;
	default:
		return EXCEPTION_CONTINUE_SEARCH;
	}
	char title[128];
	snprintf( title, sizeof( title ), "exception 0x%08lX at %p",
		info->ExceptionRecord->ExceptionCode, info->ExceptionRecord->ExceptionAddress );
	writeCrashReport( title, *info->ContextRecord );
	return EXCEPTION_CONTINUE_SEARCH;
}
//-------------------------------------------------------------------------------------
static void onTerminate(void){
	// An exception nobody caught (e.g. thrown from a destructor or through a C callback).
	Ogre::String title = "std::terminate";
	try{
		if( std::current_exception() ){
			std::rethrow_exception( std::current_exception() );
		}
	}catch( Ogre::Exception &e ){
		title += ": " + e.getFullDescription();
	}catch( std::exception &e ){
		title += Ogre::String( ": " ) + e.what();
	}catch( ... ){
		title += ": unknown exception";
	}
	CONTEXT context;
	RtlCaptureContext( &context );
	writeCrashReport( title, context );
	abort();
}
//-------------------------------------------------------------------------------------
static void onAbort( int ){
	CONTEXT context;
	RtlCaptureContext( &context );
	writeCrashReport( "abort()", context );
}
#endif

#ifdef __cplusplus
	extern "C" {
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
		int main(int argc, char *argv[])
#endif
		{
			int exitCode = 0;
			try {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
				AddVectoredExceptionHandler( 1, onCrash );
				std::set_terminate( onTerminate );
				signal( SIGABRT, onAbort );
				DevTest::parseCommandLine( strCmdLine );
#else
				Ogre::String commandLine;
				for( int i = 1; i < argc; i++ ){
					commandLine += Ogre::String( argv[i] ) + " ";
				}
				DevTest::parseCommandLine( commandLine );
#endif
				TUMBU* app = TUMBU::getInstance();
				app->go();
				delete app;
			} catch( Ogre::Exception &e ) {
				exitCode = 1;
				if( Ogre::LogManager::getSingletonPtr() ) Ogre::LogManager::getSingletonPtr()->logError( "Fatal: " + e.getFullDescription() );
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
				if( !DevTest::isEnabled() ) MessageBox( NULL, e.getFullDescription().c_str(), "TUMBU - fatal error", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
				std::cerr << "Fatal: " <<
					e.getFullDescription().c_str() << std::endl;
#endif
			}catch( std::exception &e ){
				exitCode = 1;
				if( Ogre::LogManager::getSingletonPtr() ) Ogre::LogManager::getSingletonPtr()->logError( Ogre::String( "Fatal: " ) + e.what() );
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
				if( !DevTest::isEnabled() ) MessageBox( NULL, e.what(), "TUMBU - fatal error", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
				std::cerr << "Fatal: " << e.what() << std::endl;
#endif
			}
			return exitCode;
		}
#ifdef __cplusplus
	}
#endif