/* 
 * ======================================================================================
 * File:        frame_saver_params.h
 *
 * Purpose:     external interface (API) for Frame_Saver_Params
 * 
 * History:     1. 2016-10-29   JBendor     Created    
 *              2. 2016-10-29   JBendor     Updated 
 *              3. 2016-11-04   JBendor     Support for custom pipelines
 *              4. 2016-11-06   JBendor     Defined and used MKDIR_MODE
 *              5. 2016-11-24   JBendor     Support dynamic params update
 *              6. 2016-12-08   JBendor     Support the actual Gstreamer plugin
 *              7. 2016-12-20   JBendor     Updated
 *
 * Copyright (c) 2016 TELMATE INC. All Rights Reserved. Proprietary and confidential.
 *               Unauthorized copying of this file is strictly prohibited.
 * ======================================================================================
 */

#ifndef __Frame_Saver_Params_H__

#define __Frame_Saver_Params_H__

#include <gst/gst.h>

#define  NANOS_PER_MILLISEC             ((guint64) (1000L * 1000L))
#define  NANOS_PER_SECOND               (NANOS_PER_MILLISEC * 1000)
#define  NANOS_PER_MINUTE               (NANOS_PER_SECOND * 60)
#define  NANOS_PER_HOUR                 (NANOS_PER_MINUTE * 60)
#define  NANOS_PER_DAY                  (NANOS_PER_HOUR * 24)
#define  NANOS_PER_YEAR                 (NANOS_PER_DAY * 365)
#define  NANOS_PER_MONTH                (NANOS_PER_YEAR / 12)

#define  MIN_TICKS_MILLISEC             (100)
#define  MAX_PAD_NAME_LNG               (100)
#define  MAX_ELEMENT_NAME_LNG           (100)
#define  MAX_PIPELINE_CFG_LNG           (900)
#define  MAX_PARAMS_SPECS_LNG           (4000)
#define  MAX_PARAMS_ARRAY_LNG           (20)

#define DEFAULT_VID_SRC_NAME            ("videotestsrc0")
#define DEFAULT_VID_CVT_NAME            ("videoconvert0")
#define DEFAULT_PIPELINE_NAME           ("untitledPipe0")


//=======================================================================================
// custom types
//=======================================================================================
typedef struct
{
    guint   one_tick_ms,            // timer-ticks interval as milliseconds
            one_snap_ms,            // frame-snaps interval as milliseconds
            max_wait_ms,            // wait-state-timeout as milliseconds
            max_play_ms;            // play-state-timeout as milliseconds

    guint   max_num_snaps_saved,    // maximum number of saves --- 0=unlimited
            max_num_failed_snap;    // maximum number of fails --- 0=unlimited

    gchar   folder_path[PATH_MAX + 1];

    gchar   producer_name[MAX_ELEMENT_NAME_LNG + 1];
    gchar   consumer_name[MAX_ELEMENT_NAME_LNG + 1];

    gchar   producer_out_pad_name[MAX_PAD_NAME_LNG + 1];
    gchar   consumer_inp_pad_name[MAX_PAD_NAME_LNG + 1];
    gchar   consumer_out_pad_name[MAX_PAD_NAME_LNG + 1];

    gchar   pipeline_name[MAX_ELEMENT_NAME_LNG + 1];
    gchar   pipeline_spec[MAX_PIPELINE_CFG_LNG + 1];

} SplicerParams_t;


#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


//=======================================================================================
// synopsis: result = pipeline_params_parse_caps(aCapsPtr,aFmtPtr,aWdtPtr,aHgtPtr,aBitsPtr)
//
// parse caps string --- returns 0 on success, else error
//=======================================================================================
gint pipeline_params_parse_caps(const char * aCapsPtr, 
                                gchar * aFormatPtr, 
                                gint  * aNumColsPtr, 
                                gint  * aNumRowsPtr,
                                gint  * aNumBitsPtr);


//=======================================================================================
// synopsis: is_ok = pipeline_params_parse_one(aSpecsPtr, aParamsPtr)
//
// parses one pipeline parameter for the frame_saver_filter --- returns TRUE for success
//=======================================================================================
gboolean pipeline_params_parse_one(const char * aSpecsPtr, SplicerParams_t * aParamsPtr);


//=======================================================================================
// synopsis: length = frame_saver_params_write_to_buffer(aParamsPtr, aBufferPtr, aMaxLength)
//
// writes to buffer all parameters of the frame_saver_filter --- returns length of text
//=======================================================================================
gint frame_saver_params_write_to_buffer(SplicerParams_t * aParamsPtr, char * aBufferPtr, gint aMaxLength);


//=======================================================================================
// synopsis: is_ok = frame_saver_params_write_to_file(aParamsPtr, aOutFilePtr)
//
// writes to file all parameters of the frame_saver_filter --- returns TRUE for success
//=======================================================================================
gboolean frame_saver_params_write_to_file(SplicerParams_t * aParamsPtr, FILE * aOutFilePtr);


//=======================================================================================
// synopsis: is_ok = frame_saver_params_initialize(aParamsPtr)
//
// sets default parameters for the frame_saver_filter --- returns TRUE for success
//=======================================================================================
gboolean frame_saver_params_initialize(SplicerParams_t * aParamsPtr);


//=======================================================================================
// synopsis: is_ok = frame_saver_params_parse_from_array(aParamsPtr, aArgsArray, aArgsCount)
//
// parses parameters for the frame_saver_filter --- returns TRUE for success
//=======================================================================================
gboolean frame_saver_params_parse_from_array(SplicerParams_t * aParamsPtr, char *aArgsArray[], int aArgsCount);


//=======================================================================================
// synopsis: is_ok = frame_saver_params_parse_from_text(aParamsPtr, aTextPtr)
//
// parses parameters for the frame_saver_filter --- returns TRUE for success
//=======================================================================================
gboolean frame_saver_params_parse_from_text(SplicerParams_t * aParamsPtr, char * aTextPtr);


#ifdef __cplusplus
}
#endif  // __cplusplus


#endif // __Frame_Saver_Params_H__
