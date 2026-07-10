/*
 * E R R O R . C P P
 */

#include <cstdlib>
#include <iostream>
#include "base/error.hpp"
#include "system/winapi.hpp"

#if defined(__APPLE__) || defined(__linux__)
#include <execinfo.h>
#include <unistd.h>
#define MACH_HAVE_EXECINFO 1
#endif

namespace BaseErr
{
    void TerminateOnError( const char* pMsg )
    {
        // ALWAYS_ASSERT fires even in release (-DPRODUCTION). Log the message and a
        // backtrace so the failure site is visible in the run logs before we exit.
        std::cerr << "[FATAL] ALWAYS_ASSERT: " << ( pMsg ? pMsg : "" ) << std::endl;
    #ifdef MACH_HAVE_EXECINFO
        void* frames[64];
        int count = backtrace( frames, 64 );
        backtrace_symbols_fd( frames, count, STDERR_FILENO );
    #endif
        SysWindowsAPI::messageBoxError( pMsg, "Error" );
        exit(1);
    }
}


/* End ERROR.CPP *************************************************/
