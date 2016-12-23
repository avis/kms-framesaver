/* 
 * ======================================================================================
 * File:        frame_saver_filter.h
 *
 * Purpose:     external interface (API) for code in "frame_saver_filter.c"
 * 
 * History:     1. 2016-10-29   JBendor     Created    
 *              2. 2016-11-04   JBendor     Updated 
 *              3. 2016-12-08   JBendor     Support Gstreamer Plugins
 *              4. 2016-12-21   JBendor     Updated
 *
 * Copyright (c) 2016 TELMATE INC. All Rights Reserved. Proprietary and confidential.
 *               Unauthorized copying of this file is strictly prohibited.
 * ======================================================================================
 */

#ifndef __Frame_Saver_Filter_H__

#define __Frame_Saver_Filter_H__

#include <gst/gst.h>


#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


//=======================================================================================
// synopsis: result = Frame_Saver_Filter_Attach(aPluginPtr)
//
// should be called when plugin is created --- returns 0 on success, else error
//=======================================================================================
extern int Frame_Saver_Filter_Attach(GstElement * aPluginPtr);


//=======================================================================================
// synopsis: result = Frame_Saver_Filter_Detach(aPluginPtr)
//
// should be called when plugin gets EOS --- returns 0 on success, else error
//=======================================================================================
extern int Frame_Saver_Filter_Detach(GstElement * aPluginPtr);


//=======================================================================================
// synopsis: result = Frame_Saver_Filter_Receive_Buffer(aPluginPtr, aBufferPtr, aCapsTextPtr)
//
// called at by the actual plugin upon buffer arrival --- returns GST_FLOW_OK
//=======================================================================================
extern int Frame_Saver_Filter_Receive_Buffer(GstElement * aPluginPtr, 
                                             GstBuffer  * aBufferPtr, 
                                             const char * aCapsTextPtr);


//=======================================================================================
// synopsis: result = Frame_Saver_Filter_Transition(aPluginPtr)
//
// called at by the actual plugin upon state change --- returns 0 on success, else error
//=======================================================================================
extern int Frame_Saver_Filter_Transition(GstElement * aPluginPtr, GstStateChange aTransition);


//=======================================================================================
// synopsis: result = Frame_Saver_Filter_Set_Params(aPluginPtr, aNewValuePtr, aParamSpecPtr)
//
// called at by the actual plugin to change params --- returns 0 on success, else error
//=======================================================================================
extern int Frame_Saver_Filter_Set_Params(GstElement  * aPluginPtr, 
                                         const gchar * aNewValuePtr, 
                                         gchar       * aParamSpecPtr);


//=======================================================================================
// synopsis: result = frame_saver_filter_tester(argc, argv)
//
// performs a test of the frame_saver_filter --- returns 0 on success, else error
//=======================================================================================
extern int frame_saver_filter_tester(int argc, char ** argv);


#ifdef __cplusplus
}
#endif  // __cplusplus


#endif // __Frame_Saver_Filter_H__
