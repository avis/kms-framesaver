/*
 * ======================================================================================
 * File:        wrapped_natives.h
 *
 * Purpose:     wrapper for platform's native functions (machine and OS)
 *
 * History:     1. 2016-11-05   JBendor     created from base
 *              2. 2016-11-07   JBendor     updated copyright 
 *              3. 2016-12-20   JBendor     updated
 *
 * Copyright (c) 2016 TELMATE INC. All Rights Reserved. Proprietary and confidential.
 *               Unauthorized copying of this file is strictly prohibited.
 * ======================================================================================
 */

#ifndef _WRAPPED_NATIVE_PLATFORMS_H_
#define _WRAPPED_NATIVE_PLATFORMS_H_


#if defined WIN32 || defined _WIN32 || defined __WIN32__ || defined _MSC_VER

    #ifdef _PLATFORM_IS_KNOWN_
        #error "platform redefined"
    #else
        #define _PLATFORM_IS_KNOWN_
    #endif

    #ifndef WIN32
        #define WIN32
    #endif

    #ifndef _WIN32
        #define _WIN32
    #endif

    #ifndef __WIN32__
        #define __WIN32__
    #endif

    #include <windows.h>
    #include <stdint.h>
    #include <ctype.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <limits.h>
	#include <direct.h>
    #include <sys/timeb.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <io.h>

    typedef DWORD               THREAD_RETVAL;
    typedef THREAD_RETVAL       (WINAPI *LPTHREAD_START_ROUTINE)(LPVOID);   

    #define THREAD_EXIT(v)      return( (THREAD_RETVAL) (v) )

    #ifndef snprintf
        #define snprintf        _snprintf
    #endif

    #ifndef S_IWRITE
        #define S_IWRITE  _S_IWRITE
        #define S_IREAD   _S_IREAD
    #endif

	#define MK_RW_DIR(path)     mkdir( (path) )
	#define MK_RWX_DIR(path)    mkdir( (path) )

	#define GET_CWD(buf,lng)    _getcwd( (buf),(lng) )

	#define ABS_PATH(p,b,l)     _fullpath( (b), (p), (l) )

	#ifndef PATH_MAX
		#define PATH_MAX        (260)
	#endif

	#define PATH_DELIMITER      '\\'
    #define PATH_SLASH_STR      "\\"

#endif  // WIN32


#if defined __APPLE__ || defined _OSX || defined _OSX_ || defined __OSX__
    #ifndef _LINUX_
        #define _LINUX_
    #endif
#endif


#if defined _LINUX || defined _LINUX_ || defined __LINUX__ || defined __linux__ //|| defined __GNUC__

    #ifdef _PLATFORM_IS_KNOWN_
        #error "platform redefined"
    #else
        #define _PLATFORM_IS_KNOWN_
    #endif

    #ifndef _LINUX
        #define _LINUX
    #endif

    #ifndef _LINUX_
        #define _LINUX_
    #endif

    #ifndef __LINUX__
        #define __LINUX__
    #endif

    #ifndef __linux__
        #define __linux__
    #endif

    #ifndef _GNU_SOURCE             // can affect preprocessing of <features.h>
        #define _GNU_SOURCE  1
    #endif

    #ifndef _THREAD_SAFE            // can affect preprocessing of <features.h>
        #define _THREAD_SAFE  1
    #endif

    #ifdef WINAPI
        #undef  WINAPI
    #endif
    #define WINAPI

    #include <features.h>
	#include <termios.h>
    #include <stdint.h>
	#include <unistd.h>
    #include <ctype.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <limits.h>
    #include <dirent.h>
    #include <unistd.h>
    #include <pthread.h>
    #include <time.h>
    #include <sys/timeb.h>
    #include <sys/types.h>
    #include <sys/stat.h>
	#include <glib.h>

    typedef unsigned int        UINT;
    typedef uint8_t             BYTE;
    typedef uint16_t            WORD;
    typedef uint32_t            DWORD;
    typedef void *              LPVOID;
    typedef void *              HANDLE;
    typedef void *              THREAD_RETVAL;
    typedef THREAD_RETVAL       (WINAPI *LPTHREAD_START_ROUTINE)(LPVOID);   

    #define THREAD_EXIT(v)      return((THREAD_RETVAL)(&v))

    #ifndef S_IFMT
        #define S_IWRITE  0020000
        #define S_IREAD   0010000
        #define S_IFDIR   0040000
        #define S_IFREG   0100000
		#define S_IFMT   (S_IFDIR | S_IFREG | S_IREAD | S_IWRITE)
    #endif


	extern char * realpath (const char * path_ptr, char * buff_ptr);

	#define MK_RW_DIR(path)     g_mkdir_with_parents( (path), (S_IRWXU | S_IRWXG) )
	#define MK_RWX_DIR(path)    g_mkdir_with_parents( (path), ALLPERMS )

	#define GET_CWD(buf,lng)    getcwd( (buf), (lng) )

	#define ABS_PATH(p,b,l)     realpath( (p), (b) )

	#ifndef PATH_MAX
		#define PATH_MAX        (260)
	#endif

	#define PATH_DELIMITER      '/'
    #define PATH_SLASH_STR      "/"

#endif  // _LINUX_ 


#ifndef _PLATFORM_IS_KNOWN_
    #error "unknown platform"
#endif


#ifdef __GNUC__
    #pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
    #pragma GCC diagnostic ignored "-Wformat-security"
#endif

#ifndef MAXUINT
    #define MAXUINT         ((UINT) ~((UINT) 0))
#endif

#ifndef MAXINT
    #define MAXINT          ((int)(MAXUINT >> 1))
#endif

// number of slots in array
#define ARRAY_SLOTS(a)      ( sizeof(a) / sizeof(*(a)) )

// well named substitutes for max and min macros    
#define MIN_OF_TWO(a,b)     ( (a) < (b) ? (a) : (b) )  
#define MAX_OF_TWO(a,b)     ( (a) > (b) ? (a) : (b) )


#ifdef  __cplusplus
extern "C" {
#endif


uint64_t nativeGetSysTimeMillies();                     // gets the system clock

int16_t  nativeGetRandomNum( int16_t maxValue );        // uses (~maxValue) as new seed when (maxValue < 0)

int      nativeGetFileMode( const char * pszPath );     // returns 0 iff failed, else (S_IFDIR,S_IFREG,etc)

int      nativeGetFileInfo( const char * pszPath, struct stat * ptrInfo );      // same as nativeGetFileMode

int      nativeSetFileMode( const char * pszPath, const char * pszRW );         // returns 0 iff success

int      nativeRemoveFile( const char * pszPath, int maxNumRetries );           // returns 0 iff success

int      nativeRenameFile( const char * pszPathNow, const char * pszNewPath );  // returns 0 iff success

int      nativeGetTempPath( char * pszFolderPath, int maxPathLength );          // returns 0 iff failed

int      nativeSleepMillies( int numThreadSleepMillies );                       // returns 0 iff success

int      nativeDeleteThread( HANDLE nativeThreadHandle );                       // returns 0 iff success

int      nativeCreateThread( HANDLE* ptrThreadHandle, LPTHREAD_START_ROUTINE ptrCallback, LPVOID ptrParam );

int      nativeCreateMutex ( HANDLE* ptrNewMutexHandle );                       // returns 0 iff success

int      nativeDeleteMutex ( HANDLE nativeMutexHandle );                        // returns 0 iff success

int      nativeReleaseMutex ( HANDLE nativeMutexHandle );                       // returns 0 iff success

int      nativeTryLockMutex ( HANDLE mutexHandle, int timeoutMillies );         // returns 0 iff success

char*    nativeStrDup(const char * pszText);             // Replaces "strdup" which is availale in POSIX

void     nativeWaitForKeypress();          // WINDOWS: waits for any key --- UNIX: waits only for Ctrl-C


#ifdef  __cplusplus
}
#endif


#endif // _WRAPPED_NATIVE_PLATFORMS_H_

