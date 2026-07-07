/*
 * W I N A P I . C P P
 * (c) Charybdis Limited, 1998. All Rights Reserved
 */

//  Definitions of non-inline non-template methods and global functions

#include "system/winapi.hpp"
#include <SDL2/SDL.h>

SysWindowsAPI::SysWindowsAPI()
{

    TEST_INVARIANT;
}

SysWindowsAPI::~SysWindowsAPI()
{
    TEST_INVARIANT;

}

void SysWindowsAPI::CLASS_INVARIANT
{
    INVARIANT( this != NULL );
}

ostream& operator <<( ostream& o, const SysWindowsAPI& t )
{

    o << "SysWindowsAPI " << (void*)&t << " start" << std::endl;
    o << "SysWindowsAPI " << (void*)&t << " end" << std::endl;

    return o;
}

//static
void SysWindowsAPI::sleep( double milliseconds )
{
	//Was a no-op after the SDL port, which turned every wait loop that calls this
	//(network lobby waits, loader spins, message boxes) into a 100%-CPU busy loop and
	//starved OS network scheduling. Restore the intended behaviour with SDL_Delay.
	if( milliseconds < 0.0 )
		milliseconds = 0.0;
	SDL_Delay( _STATIC_CAST( Uint32, milliseconds ) );
}

//static
void SysWindowsAPI::messageBox( const char* pMessage, const char* pTitle )
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, pTitle, pMessage, NULL);
}

//static
void SysWindowsAPI::messageBoxError( const char* pMessage, const char* pTitle )
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, pTitle, pMessage, NULL);
}

//static
void SysWindowsAPI::peekMessage()
{

}

/* End WINAPI.CPP ***************************************************/
