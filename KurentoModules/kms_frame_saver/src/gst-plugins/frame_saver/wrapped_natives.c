/*
 * ======================================================================================
 * File:        wrapped_natives.h
 *
 * Purpose:     wrapper for platform's native functions (machine and OS)
 *
 * History:     1. 2016-11-05   JBendor     created from base
 *              2. 2016-11-06   JBendor     updated copyright 
 *              3. 2016-12-20   JBendor     updated
 *
 * Copyright (c) 2016 TELMATE INC. All Rights Reserved. Proprietary and confidential.
 *               Unauthorized copying of this file is strictly prohibited.
 * ======================================================================================
 */

#include "wrapped_natives.h"


//=======================================================================================
// nativeGetSysTimeMillies() --- gets the system clock
//=======================================================================================
uint64_t nativeGetSysTimeMillies()
{
    struct timeb tmp_time_now;

    ftime( & tmp_time_now );

    return (1000L * (uint64_t) tmp_time_now.time) + (uint64_t) tmp_time_now.millitm;
}


//=======================================================================================
// nativeGetRandomNum() --- uses (~maxValue) as new seed when (maxValue < 0)
//=======================================================================================
int16_t nativeGetRandomNum( int16_t maxValue )
{
    static int16_t g_max_rand_num = (int16_t) (RAND_MAX & 0x7FFF);
    static int16_t g_new_rand_num = 0;

    if (maxValue < 0)
    {
        srand ( (unsigned int) ~maxValue );    // use-new-seed
    }

    g_new_rand_num = (int16_t) (rand() & 0x7FFF);

    if (maxValue == 0)
    {
        return (int16_t) g_new_rand_num;
    }

    return (int16_t) ( ( ((int) g_new_rand_num) * ((int) maxValue) ) / ((int) g_max_rand_num) );
}


//=======================================================================================
// nativeSleepMillies --- suspend current thread for designated time
//=======================================================================================
int nativeSleepMillies( int numMillies )
{
    if (numMillies >= 0)
    {
        #ifdef WIN32
            Sleep( (unsigned) numMillies );
        #else
            #ifdef usleep
                usleep( 1000L * numMillies );
            #else
                struct timespec request, remain;
                request.tv_sec  = numMillies / 1000;
                request.tv_nsec = (1000 * 1000) * (numMillies - (request.tv_sec * 1000));
                nanosleep( &request, &remain );
            #endif
        #endif

        return 0;
    }
    
    return -1;
}


//=======================================================================================
// nativeDeleteThread() 
//=======================================================================================
int nativeDeleteThread( HANDLE nativeThreadHandle )
{
    if ( nativeThreadHandle == NULL )
    {
        return -1;
    }

    #ifdef WIN32
        CloseHandle( nativeThreadHandle );
    #endif

    #ifdef _LINUX_
        pthread_join( (pthread_t) nativeThreadHandle, (void **) NULL );
    #endif

    return 0;
}


//=======================================================================================
// nativeCreateThread() 
//=======================================================================================
int nativeCreateThread( HANDLE* ptrThreadHandle, LPTHREAD_START_ROUTINE ptrCallback, LPVOID ptrParam )
{
    #ifdef WIN32

        DWORD thread_number;
        
        HANDLE thread_handle = CreateThread(NULL,
                                            0,            // default stack length,
                                            (LPTHREAD_START_ROUTINE) ptrCallback,
                                            ptrParam,
                                            0,            // default thread flags,
                                            &thread_number);
        if (thread_handle != NULL)
        {
            *ptrThreadHandle = thread_handle;
            return 0;
        }

    #endif  // WIN32

    #ifdef _LINUX_

        pthread_t  thread_handle;

        if ( pthread_create(&thread_handle, NULL, (LPTHREAD_START_ROUTINE) ptrCallback, ptrParam) == 0 )
        {
            *ptrThreadHandle = (HANDLE) thread_handle;
            return 0;
        }

    #endif  // _LINUX_

    *ptrThreadHandle = NULL;

    return -1;
}


//=======================================================================================
// nativeGetFileMode() --- returns 0 iff failed, else (S_IFDIR,S_IFREG,etc)
//=======================================================================================
int nativeGetFileMode( const char * pszPath )
{
    struct stat file_info;

    return nativeGetFileInfo( pszPath, &file_info );
}


//=======================================================================================
// nativeGetFileInfo() --- returns 0 iff failed, else mode (S_IFDIR,S_IFREG,etc)
//=======================================================================================
int nativeGetFileInfo( const char * pszPath, struct stat * ptrInfo )
{
    int is_ok = (stat(pszPath, ptrInfo) == 0);

    return ( is_ok ? (int) (ptrInfo->st_mode & S_IFMT) : 0 ) ;
}


//=======================================================================================
// nativeSetFileMode() --- returns 0 iff success
//=======================================================================================
int nativeSetFileMode( const char * pszPath, const char * pszRW )
{
    int is_readable = (strchr(pszRW, 'r') != NULL) || (strchr(pszRW, 'R') != NULL);

    int is_writable = (strchr(pszRW, 'w') != NULL) || (strchr(pszRW, 'W') != NULL);

    int  mode_flags = (is_readable ? S_IREAD : 0) | (is_writable ? S_IWRITE : 0);

    #ifdef _chmod
        return _chmod ( pszPath, mode_flags );
    #else
        return  chmod ( pszPath, mode_flags );
    #endif
}


//=======================================================================================
// nativeRemoveFile() --- returns 0 iff success
//=======================================================================================
int nativeRemoveFile( const char * pszPath, int maxNumRetries )
{
    int error_code = remove(pszPath);

    if ( (error_code != 0) && (nativeGetFileMode(pszPath) == 0) )
    {
        return 0;   // file does not exist
    }

    while ( (error_code != 0) && (--maxNumRetries >= 0) )
    {
        nativeSetFileMode(pszPath, "W");

        error_code = remove(pszPath);

        nativeSleepMillies(1);
    }

    return error_code;
}


//=======================================================================================
// nativeRenameFile() --- returns 0 iff success
//=======================================================================================
int nativeRenameFile( const char * pszPathNow, const char * pszNewPath )
{
   int error_code = rename(pszPathNow, pszNewPath);

   return error_code;
}


//=======================================================================================
// nativeGetTempPath() --- returns 0 iff failed
//=======================================================================================
int nativeGetTempPath( char * pszFolderPath, int maxPathLength )
{
    int length = 0;

    if ( (pszFolderPath != NULL) && (maxPathLength > 9) )
    {
        #ifdef WIN32
            length = (int) GetTempPath( (UINT) (maxPathLength - 1), pszFolderPath );
            if ((length > 0) && (pszFolderPath[length-1] != '\\'))
            {
                pszFolderPath[length-1] = '\\';    pszFolderPath[++length] = 0;
            }
        #endif

        #ifdef _LINUX_   
            length = sprintf(pszFolderPath, "%s", "/tmp/");
        #endif
    }

    return ( (length > 1) ? length : 0 );
}


#ifdef _CYGWIN
//=======================================================================================
// realpath() --- returns NULL iff failed
//=======================================================================================
char * realpath( const char *aPathPtr, char * aBuffPtr )
{
    int lng = (int) strlen( GET_CWD(aBuffPtr, PATH_MAX) );

    if (aPathPtr[0] == PATH_DELIMITER)
    {
        sprintf(aBuffPtr, "%s", aPathPtr);  return (aBuffPtr);
    }

    while ((lng > 0) && (aPathPtr[0] == '.') && (aPathPtr[1] == '.') && (aPathPtr[2] == PATH_DELIMITER))
    {
        while ((--lng > 0) && (aBuffPtr[lng] != PATH_DELIMITER))
        {
            ; // next step back
        }
        aPathPtr += 3;
    }

    if ((aPathPtr[0] == '.') && (aPathPtr[1] == PATH_DELIMITER))
    {
        lng += sprintf(aBuffPtr+lng, "%s", ++aPathPtr);   return (aBuffPtr);
    }

    lng += sprintf(aBuffPtr+lng, "%s", aPathPtr);

    return (aBuffPtr);  // may need more work
}
#endif // _CYGWIN


//=======================================================================================
// nativeCreateMutex() --- returns 0 iff success
//=======================================================================================
int nativeCreateMutex ( HANDLE* ptrNewMutexHandle )
{
    #ifdef WIN32
        HANDLE ptr_native_mutex = CreateMutex( NULL, FALSE, NULL );  // mutex has recursive property
    #endif

    #ifdef _LINUX_
        pthread_mutex_t * ptr_native_mutex = (pthread_mutex_t*) calloc( 1, sizeof(pthread_mutex_t) );

        #ifdef _CYGWIN
            pthread_mutexattr_t  mutex_type_union;
            pthread_mutexattr_settype( &mutex_type_union, (int) PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP );
        #else
            pthread_mutexattr_t  mutex_type_union = { {0, 0, 0, PTHREAD_MUTEX_RECURSIVE_NP} };
        #endif    


        if ( pthread_mutex_init( ptr_native_mutex, &mutex_type_union) != 0 )
        {
            pthread_mutex_destroy ( ptr_native_mutex );
            free( ptr_native_mutex );
            ptr_native_mutex = NULL ;
        }        
    #endif

    *ptrNewMutexHandle = (HANDLE) ptr_native_mutex;

    return (ptr_native_mutex != NULL) ? 0 : -1;
}


//=======================================================================================
// nativeDeleteMutex() --- returns 0 iff success
//=======================================================================================
int nativeDeleteMutex(HANDLE mutexHandle)
{
    int error_code = 0;

    if (mutexHandle != NULL)
    {
        nativeReleaseMutex(mutexHandle);

        #ifdef WIN32
            error_code = CloseHandle(mutexHandle) ? 0 : -1;
        #endif

        #ifdef _LINUX_
            error_code = pthread_mutex_destroy ( (pthread_mutex_t*) mutexHandle ) ? -1 : 0;
            free( (void*) mutexHandle );
        #endif
    }

    return error_code;
}


//=======================================================================================
// nativeReleaseMutex() --- returns 0 iff success
//=======================================================================================
int nativeReleaseMutex(HANDLE mutexHandle)
{
    int error_code = 0;

    if (mutexHandle != NULL)
    {
        #ifdef WIN32
            error_code = ReleaseMutex(mutexHandle) ? 0 : -1;
        #endif

        #ifdef _LINUX_
            error_code = pthread_mutex_unlock((pthread_mutex_t*) mutexHandle);
        #endif
    }

    return error_code;
}


//=======================================================================================
// nativeTryLockMutex() --- returns 0 iff success
//=======================================================================================
int  nativeTryLockMutex ( HANDLE mutexHandle, int timeoutMillies )
{
    DWORD max_wait_ms = (timeoutMillies < 0) ? (DWORD) MAXUINT : (DWORD) timeoutMillies;

    int    error_code = 0;

    if (mutexHandle != NULL)
    {
        #ifdef WIN32
            DWORD wait_result = WaitForSingleObject(mutexHandle, max_wait_ms);

            error_code = (wait_result == WAIT_OBJECT_0) ? 0 : -1;
        #endif

        #ifdef _LINUX_
            uint64_t  end_wait_ms = nativeGetSysTimeMillies() + max_wait_ms;

            for ( ; ; )
            {
                if ( pthread_mutex_lock((pthread_mutex_t*) mutexHandle) == 0 )
                {
                    error_code = 0;   break;
                } 
                if ( nativeGetSysTimeMillies() >= end_wait_ms )
                {
                    break;
                }
                nativeSleepMillies( 1 );
            }
        #endif
    }

    return error_code;
}


//=======================================================================================/////
// nativeWaitForKeypress --- WIN32: waits for any keypess. UNIX: waits only for Ctrl-C 
//=======================================================================================/////
void nativeWaitForKeypress()
{
    #ifdef _kbhit
        while ( !_kbhit() )
        {
            continue;
        }
    #else
        while ( getchar() != EOF ) 
        {
            continue;
        }
    #endif

    return;
}


//=======================================================================================
// copied_string_ptr = nativeStrDup(psz_chars)  --- Replacing "strdup" which is availale in POSIX
//=======================================================================================
char * nativeStrDup(const char * psz_chars)
{
    #ifdef strdup
        char * psz_alloc = strdup(psz_chars);
    #else
        char * psz_alloc = malloc(strlen(psz_chars) + 1);
        if (psz_alloc != NULL)
        {
            strcpy(psz_alloc, psz_chars);
        }
    #endif

    return psz_alloc;
}
