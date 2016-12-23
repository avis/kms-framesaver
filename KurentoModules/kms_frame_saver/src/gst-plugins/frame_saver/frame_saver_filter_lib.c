/* 
 * ======================================================================================
 * File:        frame_saver_filter_lib.c
 *
 * Purpose:     implements the API for the Frame_Saver_Filter_Library (aka FSL)
 * 
 * History:     1. 2016-10-14   JBendor     Created
 *              2. 2016-12-20   JBendor     Updated
 *
 * Copyright (c) 2016 TELMATE INC. All Rights Reserved. Proprietary and confidential.
 *               Unauthorized copying of this file is strictly prohibited.
 * ======================================================================================
 */

#include "frame_saver_filter_lib.h"
#include "frame_saver_filter.h"
#include <gst/gst.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


static guint s_GST_ver_major, s_GST_ver_minor, s_GST_ver_micro, s_GST_ver_nano;

static char s_FSL_Version[200] = { 0 };


//=======================================================================================
// synopsis: (void) do_wait_for_keypress()
//
// WIN32: waits for any keypess. UNIX: waits only for Ctrl-C 
//=======================================================================================
static void do_wait_for_keypress()
{
#if defined _LINUX || defined __GNUC__
    printf("please press Ctrl-C ... ");
    while ( getchar() != EOF )
    {
        continue;
    }
#else
    extern int _kbhit(void);
    printf("please press any key ... ");
    while ( !_kbhit() )
    {
        continue;
    }
#endif
    return;
}


//=======================================================================================
// synopsis: psz_version_text = fsl_get_version()
//
// gets version info about Gstreamer and FSL
//=======================================================================================
API_LINKAGE const char * fsl_get_version()
{
    return s_FSL_Version;
}


//=======================================================================================
// synopsis: result = fsl_initialize()
//
// initializes the library --- returns 0 on success, else error
//=======================================================================================
API_LINKAGE int32_t fsl_initialize()
{
#ifdef _IS_LIB_FOR_PLUGIN_
    #define _VERSION "FrameSaverPlugin: 1.0.0"
#else
    #define _VERSION "FrameSaverLibrary 1.0.0"
#endif

    gst_init(NULL, NULL);

    gst_version(&s_GST_ver_major, &s_GST_ver_minor, &s_GST_ver_micro, &s_GST_ver_nano);

    snprintf(s_FSL_Version, 
             sizeof(s_FSL_Version) / sizeof(*s_FSL_Version),
             "%s %s %s --- Current GStreamer version: %d.%d.%d.%d\n", 
             _VERSION, 
             __DATE__, 
             __TIME__,
             s_GST_ver_major, 
             s_GST_ver_minor, 
             s_GST_ver_micro, 
             s_GST_ver_nano);

    char * ptr_date = s_FSL_Version + strlen(_VERSION) + 1;

    ptr_date[3] = ptr_date[6] = '-';

    return 0;
}


//=======================================================================================
// synopsis: test_result = fsl_main_test(argc, argv)
//
// performs a test on the FSL library --- returns 0
//=======================================================================================
API_LINKAGE int32_t fsl_main_test(int32_t argc, char** argv)
{
    printf( "%s%s", "\n", "fsl_main_test() started \n" );

    fsl_initialize();

    printf( "%s", fsl_get_version() );

    printf( "fsl_main_test() result=(0x%X) \n", frame_saver_filter_tester(argc, argv) );

    printf( "fsl_main_test() ended \n\n" );

    do_wait_for_keypress();
    
    return 0;
}


#ifdef __cplusplus
}
#endif  // __cplusplus
