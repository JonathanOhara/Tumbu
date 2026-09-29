#include "TUMBU.h"
#include <exception>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
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
			try {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
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
				if( Ogre::LogManager::getSingletonPtr() ) Ogre::LogManager::getSingletonPtr()->logError( "Fatal: " + e.getFullDescription() );
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
				if( !DevTest::isEnabled() ) MessageBox( NULL, e.getFullDescription().c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
				std::cerr << "An exception has occured: " <<
					e.getFullDescription().c_str() << std::endl;
#endif
			}catch( std::exception &e ){
				if( Ogre::LogManager::getSingletonPtr() ) Ogre::LogManager::getSingletonPtr()->logError( Ogre::String( "Fatal: " ) + e.what() );
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
				if( !DevTest::isEnabled() ) MessageBox( NULL, e.what(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
				std::cout<<"!!!!std::exception!!!!"<<e.what()<<std::endl;
#endif
			}
			return 0;
		}
#ifdef __cplusplus
	}
#endif