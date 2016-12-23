/* 
 * ======================================================================================
 * File:        frame_saver_filter_lib.h
 *
 * Purpose:     external interface (API) for Frame_Saver_Filter_Library (aka FSL)
 * 
 * History:     1. 2016-10-14   JBendor     Created    
 *              2. 2016-12-20   JBendor     Updated
 *
 * Copyright (c) 2016 TELMATE INC. All Rights Reserved. Proprietary and confidential.
 *               Unauthorized copying of this file is strictly prohibited.
 * ======================================================================================
 */

#ifndef __Frame_Saver_Filter_Lib_H__

#define __Frame_Saver_Filter_Lib_H__


#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


#if defined _MSC_VER && defined _USRDLL
    #ifdef _USRDLL
        #define API_LINKAGE  __declspec(dllexport)
    #else
        #define API_LINKAGE  __declspec(dllimport)
    #endif
#else
    #define API_LINKAGE
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


//=======================================================================================
// custom types
//=======================================================================================
typedef void    *fsl_handle_t;
typedef uint8_t  fsl_bool_t;
typedef int32_t  fsl_error_t;


//=======================================================================================
// useful macros
//=======================================================================================
#define FSL_TRUE                        (1)
#define FSL_FALSE                       (0)
#define FSL_IS_SUCCESS(error_code)      ((error_code) == 0)
#define FSL_IS_HANDLE_VALID(tsv_handle) ((fsl_handle) != NULL)


//=======================================================================================
// synopsis: psz_version_text = fsl_get_version()
//
// gets version info about Gstreamer and FSL
//=======================================================================================
API_LINKAGE const char * fsl_get_version();


//=======================================================================================
// synopsis: result = fsl_initialize()
//
// initializes the library --- returns 0 on success, else error
//=======================================================================================
API_LINKAGE int32_t fsl_initialize();


//=======================================================================================
// synopsis: result = fsl_main_test(argc,argv)
//
// performs a self-test on the library --- returns 0
//=======================================================================================
API_LINKAGE int32_t fsl_main_test(int32_t argc, char** argv);


#ifdef __cplusplus
}
#endif  // __cplusplus


#endif // __Frame_Saver_Filter_Lib_H__
