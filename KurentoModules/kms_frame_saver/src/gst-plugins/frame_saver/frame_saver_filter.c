/*
 * ======================================================================================
 * File:        frame_saver_filter.c
 *
 * History:     1. 2016-10-14   JBendor     Created
 *              2. 2016-10-28   JBendor     Updated
 *              3. 2016-10-29   JBendor     Removed parameters code to new file
 *              4. 2016-11-04   JBendor     Support for making custom pipelines
 *              5. 2016-12-08   JBendor     Support Gstreamer Plugins
 *              6. 2016-12-22   JBendor     Updated
 *              7. 2017-02-23   JBendor     Prevent BGR-to-RGB from modifing image frame
 *
 * Description: Uses the Gstreamer TEE to splice one video source into two sinks.
 *
 *              The baseline TEE example code is in this URL:
 *              http://tordwessman.blogspot.com/2013/06/gstreamer-tee-code-example.html
 *
 *              The following URL describes Gstreamer streaming:
 *              http://www.einarsundgren.se/gstreamer-basic-real-time-streaming-tutorial
 *
 * Copyright (c) 2016 TELMATE INC. All Rights Reserved. Proprietary and confidential.
 *               Unauthorized copying of this file is strictly prohibited.
 * ======================================================================================
 */

#include "wrapped_natives.h"

#include "frame_saver_filter.h"
#include "frame_saver_params.h"
#include "save_frames_as_png.h"

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <glib.h>
#include <glib/gtypes.h>


#if (GST_VERSION_MAJOR == 0)
    #error "GST_VERSION_MAJOR must not be 0"
#endif

#define _DO_DBG_TRACE
#define _DO_BUS_TRACE

#define PREFIX_FORMAT           "@FrameSaver.%u --- "
#define INFINIT_NANOS           (NANOS_PER_DAY + 9);
#define NUM_APP_SINK_BUFFERS    (2)

#define CAPS_FOR_AUTO_SOURCE    "video/x-raw, width=(int)500, height=(int)200" //, framerate=(fraction)1/2"
#define CAPS_FOR_VIEW_SINKER    "video/x-raw, width=(int)500, height=(int)200"
#define CAPS_FOR_SNAP_SINKER    "video/x-raw, format=(string)RGB, bpp=(int)24"


//=======================================================================================
// custom types
//=======================================================================================
typedef enum
{
    e_PIPELINE_MAKER_ERROR_NONE = 0,
    e_PIPELINE_PARSER_HAS_ERROR = 1,
    e_FAILED_MAKE_MINI_PIPELINE = e_PIPELINE_PARSER_HAS_ERROR,
    e_FAILED_MAKE_PIPELINE_NAME = 2,
    e_FAILED_SET_PIPELINE_STATE = 3,
    e_FAILED_FETCH_PIPELINE_BUS = 4,
    e_FAILED_MAKE_WORK_ELEMENTS = 5,
    e_FAILED_LINK_MINI_PIPELINE = 6,
    e_ERROR_PIPELINE_TEE_PARAMS = 7

} PIPELINE_MAKER_ERROR_e;


typedef enum
{
    e_TEE_INSERT_ENDED = 0,
    e_TEE_INSERT_WAITS = 1,
    e_TEE_INSERT_ERROR = 2,
    e_FAIL_UNLINK_PADS = 3,
    e_FAIL_TEE_IN_PADS = 4,
    e_FAIL_TEE_UP_LINK = 5,
    e_FAIL_TEE_Q1_LINK = 6,
    e_FAIL_TEE_Q2_LINK = 7,
    e_FAIL_TEE_TO_FILE = 8

} e_TEE_INSERT_STATE_e;


typedef enum
{
    e_SPLICER_STATE_NONE = 0,
    e_SPLICER_STATE_BUSY = 1,
    e_SPLICER_STATE_DONE = 2,
    e_SPLICER_STATE_USED = 3,
    e_SPLICER_STATE_FAIL = 4

} SPLICER_STATE_e;


typedef struct _FlowSplicer_t
{
    SPLICER_STATE_e status;

    GQueue          effects_queue;

    SplicerParams_t params;

    GstCaps       * ptr_all_from_caps,
                  * ptr_all_into_caps;

    GstPad        * ptr_from_pad;           // producer-out-pad feeds consumer-inp-pad --- never NULL
    GstPad        * ptr_into_pad;           // consumer-inp-pad feeds consumer-out-pad --- never NULL
    GstPad        * ptr_next_pad;           // consumer-out-pad feeds next-element-inp-pad -- NULL OK

    GstElement    * ptr_from_element;
    GstElement    * ptr_into_element;
    GstElement    * ptr_snaps_pipeline;

    GstElement    * ptr_current_effect;
    gchar        ** pp_effects_names;

} FlowSplicer_t;


typedef struct _FramesSaver_t
{
    gint          instance_ID;              // ZERO indicates structure is empty

    GstElement  * parent_pipeline_ptr,      // NULL indicates pipline is unknown
                * attached_plugin_ptr;      // NULL indicates no plugin attached

    GstElement  * vid_sourcer_ptr,
                * video_sink1_ptr,
                * video_sink2_ptr,
                * vid_convert_ptr,
                * cvt_element_ptr,
                * tee_element_ptr,
                * Q_1_element_ptr,
                * Q_2_element_ptr;

    GstBus      * bus_ptr;                  // Bus transfers messages from/to pipeline

    GstCaps     * source_caps_ptr,          // for Source-to-Sink1 in default pipeline
                * sinker_caps_ptr;          // for TEE-to-Queue2-to-Sink2 (or appsink)

    guint           num_snap_signals,       // count of signals to snap frames
                    num_saved_frames,       // count of frames saved as files
                    num_saver_errors,       // count of frames saver's errors
                    num_stream_frames,      // count of stream input frames
                    num_stream_errors;      // count of stream input errors

    GstClockTime    frame_snap_wait_ns;     // wait time for next frame snap --- 0 is infinite
    GstClockTime    wait_state_ends_ns;     // timestamp for a TEE insertion --- 0 is infinite

    GstAppSinkCallbacks appsink_callbacks;

    FlowSplicer_t       flow_splicer_info;

    gchar               work_folder_path[PATH_MAX + 1];

} FramesSaver_t;


#define MAX_NUM_PLUGINS (4000)

static FramesSaver_t    The_FramesSavers_Array[ MAX_NUM_PLUGINS ] = { { 0, NULL, NULL } };

static const int        MUTEX_TIMEOUT_MS = 10;

static void          *  The_Mutex_Handle = NULL;

static GMainLoop     *  The_MainLoop_Ptr = NULL;

static GstClock      *  The_SysClock_Ptr = NULL;

static GstClockTime    The_LaunchTime_ns = 0;

static gint            The_Plugins_Count = -1;


//=======================================================================================
// synopsis: splicer_ptr = do_get_splicer_ptr(aSaverPtr)
//
// returns pointer to structure: FlowSplicer_t
//=======================================================================================
static void do_DBG_print(const gchar * aNoticePtr, FramesSaver_t * aSaverPtr)
{
	#ifndef _NO_DBG_TRACE
		GST_DEBUG(PREFIX_FORMAT "%s", (aSaverPtr ? aSaverPtr->instance_ID : 0), aNoticePtr);
	#endif

	return;
}

//=======================================================================================
// synopsis: splicer_ptr = do_get_splicer_ptr(aSaverPtr)
//
// returns pointer to structure: FlowSplicer_t
//=======================================================================================
static FlowSplicer_t * do_get_splicer_ptr( FramesSaver_t * aSaverPtr )
{
    return & aSaverPtr->flow_splicer_info;
}


//=======================================================================================
// synopsis: result = do_initialize_static_resources()
//
// initializes the necessary resources --- returns 0 on success
//=======================================================================================
static int do_initialize_static_resources()
{
    if ( (The_SysClock_Ptr == NULL) || (The_Plugins_Count < 0) )    // once only or RESET
    {
        The_SysClock_Ptr  = gst_system_clock_obtain();

        The_LaunchTime_ns = gst_clock_get_time( The_SysClock_Ptr );
    }

    memset( The_FramesSavers_Array, 0, sizeof(The_FramesSavers_Array) );

    The_Plugins_Count = 0;

    if (nativeCreateMutex(&The_Mutex_Handle) != 0)
    {
        The_Mutex_Handle = NULL;    // create failed
    }

    return ( (The_Mutex_Handle != NULL) ? 0 : -1 );
}


//=======================================================================================
// synopsis: result = do_find_plugin_index(aPluginPtr)
//
// returns index in array of known plugins --- returns -1 iff not in the array
//=======================================================================================
static int do_find_plugin_index(const void * aPluginPtr)
{
    int index = 0;

    // possibly --- once only or RESET --- initialize resources
    if ( (The_Mutex_Handle == NULL) || (The_Plugins_Count < 0) )
    {
        do_initialize_static_resources();

        return -1;
    }

    while ( (index < MAX_NUM_PLUGINS) && (The_FramesSavers_Array[index].attached_plugin_ptr != aPluginPtr) )
    {
        ++index;   // next
    }

    return ( (index < MAX_NUM_PLUGINS) && (aPluginPtr != NULL) ) ? index : -1;
}


//=======================================================================================
// synopsis: result = do_save_frame_buffer(aBufferPtr, aCapsPtr, aSaverPtr)
//
// snaps and saves a frame --- returns GST_FLOW_OK on success, else GST_FLOW_ERROR
//=======================================================================================
static gint do_save_frame_buffer(GstBuffer     * aBufferPtr,
                                 const char    * aCapsPtr,
                                 FramesSaver_t * aSaverPtr)
{
    /*
    * NOTE-1: image height can depend on the pixel-aspect-ratio of the source.
    *
    * NOTE-2: stride of video buffers is rounded to the nearest multiple of 4.
    */

    GstMapInfo map;

    char sz_image_format[100],
         sz_image_path[PATH_MAX + 100];

    const char * interlace = aCapsPtr ? strstr(aCapsPtr, "interlace-mode=") : (aCapsPtr = "?");

    int  cols = 0,
         rows = 0,
         bits = 8,
         errs = pipeline_params_parse_caps(aCapsPtr,
                                           sz_image_format,
                                           &cols,
                                           &rows,
                                           &bits);

    if ( (errs != 0) || (rows < 1) || (cols < 1) )
    {
        aSaverPtr->num_saver_errors += 1;
        return GST_FLOW_ERROR;  // invalid attributes
    }

    if ( (interlace != NULL) && (strstr(interlace, "progressive") == NULL) )
    {
        aSaverPtr->num_saver_errors += 1;
        return GST_FLOW_ERROR;  // only "progressive" is allowed
    }

    if (TRUE != gst_buffer_map(aBufferPtr, &map, GST_MAP_READ))
    {
        aSaverPtr->num_saver_errors += 1;
        return GST_FLOW_ERROR;
    }

    void  * data_ptr = (void*) map.data;        // pointer to frame's data bytes
    int     data_lng = (int) map.size;          // total number of frame's bytes
    int     num_pixs = rows * cols;
    int     pix_size = data_lng / num_pixs;
    int       stride = GST_ROUND_UP_4(cols * pix_size);         // bytes per row

    GstClockTime now = gst_clock_get_time (The_SysClock_Ptr);

    guint elapsed_ms = (guint) ((now - The_LaunchTime_ns) / NANOS_PER_MILLISEC);

    aSaverPtr->num_saved_frames += 1;

    if ( strncmp(sz_image_format, "BGR", 3) == 0 )
    {
        data_ptr = malloc(data_lng);
        memcpy(data_ptr, map.data, data_lng);
        convert_BGR_frame_to_RGB(data_ptr, pix_size * 8, stride, cols, rows);
        sz_image_format[0] = 'R';
        sz_image_format[2] = 'B';
    }

    sprintf(sz_image_path,
            "%s%c%05u_%lu.png",
            aSaverPtr->work_folder_path, PATH_DELIMITER,
            aSaverPtr->num_saved_frames,
            (unsigned long)time(NULL)
            );

    errs = save_frame_as_PNG(sz_image_path, sz_image_format, data_ptr, data_lng, stride, cols, rows);

    if (data_ptr != map.data)
    {
        free(data_ptr);     // discard the copied image data
    }

    #ifndef _NO_DBG_TRACE
    	GST_DEBUG(PREFIX_FORMAT "playtime=%u ... Saved=(%s), Error=(%d) \n", aSaverPtr->instance_ID,
    			elapsed_ms,
				strrchr(sz_image_path, PATH_DELIMITER) + 1,
				errs);
	#endif

    gst_buffer_unmap (aBufferPtr, &map);

    if (errs != 0)
    {
        aSaverPtr->num_saver_errors += 1;
    }

    return (errs == 0) ? GST_FLOW_OK : GST_FLOW_OK;
}


//=======================================================================================
// synopsis: result = do_appsink_callback_for_new_frame(aAppSinkPtr, aContextPtr)
//
// appsink callback for new-frame events
//=======================================================================================
static GstFlowReturn do_appsink_callback_for_new_frame(GstAppSink * aAppSinkPtr,
                                                       gpointer     aContextPtr)
{
    GstFlowReturn   flow_result = GST_FLOW_OK;

    FramesSaver_t *   saver_ptr = (FramesSaver_t *) aContextPtr;

    FlowSplicer_t * splicer_ptr = do_get_splicer_ptr( saver_ptr );

    GstSample     *  sample_ptr = gst_app_sink_pull_sample(aAppSinkPtr);

    GstCaps       *    caps_ptr = sample_ptr ? gst_sample_get_caps(sample_ptr) : NULL;

    GstBuffer     *  buffer_ptr = sample_ptr ? gst_sample_get_buffer(sample_ptr) : NULL;

    // verify valid conditions
    if ( (buffer_ptr == NULL) || (caps_ptr == NULL) || (saver_ptr == NULL) )
    {
        saver_ptr->num_stream_errors += 1;

        gst_sample_unref(sample_ptr);

        gst_caps_unref(caps_ptr);

        return GST_FLOW_ERROR;
    }

    saver_ptr->num_stream_frames += 1;

    if (splicer_ptr->params.one_snap_ms > 0)
    {
        guint total_done = saver_ptr->num_saver_errors + saver_ptr->num_saved_frames;

        if (saver_ptr->num_snap_signals > total_done)
        {
            gchar * psz_caps = gst_caps_to_string(caps_ptr);

            flow_result = do_save_frame_buffer(buffer_ptr, psz_caps, saver_ptr);

            g_free(psz_caps);
        }
    }

    gst_sample_unref(sample_ptr);

    gst_caps_unref(caps_ptr);

    return flow_result;
}


//=======================================================================================
// synopsis: result = do_appsink_callback_probe_inp_pad_BUF(aPadPtr, aInfoPtr, aCtxPtr)
//
// appsink callback for input-pad BUFFER events
//=======================================================================================
static GstPadProbeReturn do_appsink_callback_probe_inp_pad_BUF(GstPad          * aPadPtr,
                                                               GstPadProbeInfo * aInfoPtr,
                                                               gpointer          aCtxPtr)
{
    FramesSaver_t *   saver_ptr = (FramesSaver_t *) aCtxPtr;

    FlowSplicer_t * splicer_ptr = do_get_splicer_ptr( saver_ptr );

    GstBuffer    * buffer_ptr = GST_PAD_PROBE_INFO_BUFFER(aInfoPtr);

    GstEvent     *  event_ptr = GST_PAD_PROBE_INFO_DATA(aInfoPtr);

    GstEventType  event_type = GST_EVENT_TYPE(event_ptr);

    if ( (buffer_ptr == NULL) || (event_type == GST_EVENT_UNKNOWN) )
    {
        return GST_PAD_PROBE_OK;
    }

#ifdef _USE_APPSINK_CALLBACKS_
    if ( saver_ptr->attached_plugin_ptr == NULL )
    {
        // GST_DEBUG_OBJECT(aPadPtr, "appsink-inp-pad --- BUF event !!! ");
        return GST_PAD_PROBE_OK;
    }
#endif

    saver_ptr->num_stream_frames += 1;

    // note: "buffer" here --- "sample" in do_appsink_callback_for_new_frame()
    if ( (splicer_ptr->params.one_snap_ms > 0) &&
         (saver_ptr->num_snap_signals > saver_ptr->num_saved_frames) )
    {
        GstCaps * caps_ptr = gst_pad_get_current_caps(aPadPtr);

        gchar   * psz_caps = gst_caps_to_string(caps_ptr);

        do_save_frame_buffer( buffer_ptr, psz_caps, saver_ptr );

        gst_caps_unref(caps_ptr);

        g_free(psz_caps);
    }

    return GST_PAD_PROBE_OK;
}


//=======================================================================================
// synopsis: result = do_appsink_callback_probe_inp_pad_EOS(aPadPtr, aInfoPtr, aCtxPtr)
//
// appsink callback for input-pad events --- se for EOS
//=======================================================================================
static GstPadProbeReturn do_appsink_callback_probe_inp_pad_EOS(GstPad          * aPadPtr,
                                                               GstPadProbeInfo * aInfoPtr,
                                                               gpointer          aCtxPtr)
{
    GstEvent* event_data_ptr = GST_PAD_PROBE_INFO_DATA(aInfoPtr);

    GstEventType  event_type = GST_EVENT_TYPE(event_data_ptr);

    if (event_type != GST_EVENT_EOS)
    {
        return GST_PAD_PROBE_PASS;
    }

    GST_DEBUG_OBJECT(aPadPtr, "appsink-inp-pad --- EOS event !!! ");

    return GST_PAD_PROBE_OK;
}


//=======================================================================================
// synopsis: is_ok = do_appsink_trigger_next_frame_snap(aSaverPtr, elapsedPlaytimeMillis)
//
// triggers frame snaps --- returns TRUE iff more snaps are allowed
//=======================================================================================
static gboolean do_appsink_trigger_next_frame_snap(FramesSaver_t * aSaverPtr, uint32_t elapsedPlaytimeMillis)
{
    FlowSplicer_t * splicer_ptr = do_get_splicer_ptr( aSaverPtr );

    GstClockTime next_snap_nanos = NANOS_PER_MILLISEC *splicer_ptr->params.one_snap_ms;

    // establish a desired time for next frame snap
    aSaverPtr->frame_snap_wait_ns += next_snap_nanos;

    // increment the number of snap-signals --- create new folder on first snap
    if (++aSaverPtr->num_snap_signals == 1)
    {
        time_t now = (unsigned long)time(NULL);

        int length = (int) strlen(aSaverPtr->work_folder_path);

        sprintf( &aSaverPtr->work_folder_path[length],
                 "%cframes_%lu",
                 PATH_DELIMITER,
                 now
                );

        int error = MK_RWX_DIR(aSaverPtr->work_folder_path);

        if (error == 0)
        {
            GST_LOG(PREFIX_FORMAT "playtime=%u %s (%s) \n", aSaverPtr->instance_ID,
                    elapsedPlaytimeMillis,
                    "... New Folder",
                    &aSaverPtr->work_folder_path[0]);
        }
        else
        {
            GST_LOG(PREFIX_FORMAT "playtime=%u %s (%s) NOT created --- error=(%d) \n", aSaverPtr->instance_ID,
                    elapsedPlaytimeMillis,
                    "... New Folder",
                    &aSaverPtr->work_folder_path[0], error);

            aSaverPtr->work_folder_path[length] = 0;

            aSaverPtr->num_snap_signals = 0;
        }

        next_snap_nanos += NANOS_PER_MILLISEC * elapsedPlaytimeMillis;

        aSaverPtr->frame_snap_wait_ns = next_snap_nanos;
    }

    gboolean is_more_snaps_ok = TRUE;

    if ((splicer_ptr->params.max_num_snaps_saved > 0) &&
        (splicer_ptr->params.max_num_snaps_saved <= aSaverPtr->num_saved_frames))
    {
        GST_DEBUG(PREFIX_FORMAT "playtime=%u ... #SAVED=%u ... Reached-Limit \n", aSaverPtr->instance_ID,
                elapsedPlaytimeMillis,
                splicer_ptr->params.max_num_snaps_saved);

         is_more_snaps_ok = FALSE;
    }
    else if ((splicer_ptr->params.max_num_failed_snap > 0) &&
             (splicer_ptr->params.max_num_failed_snap <= aSaverPtr->num_saver_errors))
    {
        GST_DEBUG(PREFIX_FORMAT "playtime=%u ... #FAILS=%u ... Reached-Limit \n", aSaverPtr->instance_ID,
                elapsedPlaytimeMillis,
                splicer_ptr->params.max_num_failed_snap);

         is_more_snaps_ok = FALSE;
    }
    else if ((aSaverPtr->attached_plugin_ptr != NULL) && (splicer_ptr->params.max_wait_ms > 0))
    {
        GST_DEBUG(PREFIX_FORMAT "playtime=%u ... #snaps=%u ... #saved=%u ... errors=%u,%u ... frames=%u\n", aSaverPtr->instance_ID,
                elapsedPlaytimeMillis,
                aSaverPtr->num_snap_signals,
                aSaverPtr->num_saved_frames,
                aSaverPtr->num_saver_errors,
                aSaverPtr->num_stream_errors,
                aSaverPtr->num_stream_frames);
    }

    return is_more_snaps_ok;
}


//=======================================================================================
// synopsis: appsink_ptr = do_appsink_create(aSaverPtr, aNamePtr, aNumBuffers)
//
// creates & configures the appsink for frame snaps --- returns element pointer
//=======================================================================================
static GstElement* do_appsink_create(FramesSaver_t * aSaverPtr, const char * aNamePtr, int aNumBuffers)
{
    do_DBG_print("do_appsink_create \n", aSaverPtr);

    GstElement * appsink_ptr = gst_element_factory_make("appsink", aNamePtr);

    aSaverPtr->appsink_callbacks.eos         = NULL;
    aSaverPtr->appsink_callbacks.new_preroll = NULL;
    aSaverPtr->appsink_callbacks.new_sample  = do_appsink_callback_for_new_frame;

#ifdef _USE_APPSINK_CALLBACKS_

    gst_app_sink_set_callbacks(GST_APP_SINK(appsink_ptr),
                               &aSaverPtr->appsink_callbacks,
                               aSaverPtr,       // pointer_to_context_of_the_callback
                               NULL);           // pointer_to_destroy_notify_callback

    gst_app_sink_set_drop(GST_APP_SINK(appsink_ptr), TRUE);

    gst_app_sink_set_emit_signals(GST_APP_SINK(appsink_ptr), TRUE);

    gst_app_sink_set_max_buffers(GST_APP_SINK(appsink_ptr), aNumBuffers);

#endif // _USE_APPSINK_CALLBACKS_

    GstPad *  ptr_inp_pad = gst_element_get_static_pad(GST_ELEMENT(appsink_ptr), "sink");

    GstPadProbeType flags = GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM;

    // install consumer-inp-pad-probe for EOS events
    gulong probe_EOS = gst_pad_add_probe( ptr_inp_pad,
                                          flags,
                                          do_appsink_callback_probe_inp_pad_EOS,
                                          aSaverPtr,
                                          NULL );

    // install consumer-inp-pad-probe for BUFFER event
    gulong probe_BUF = gst_pad_add_probe( ptr_inp_pad,
                                          GST_PAD_PROBE_TYPE_BUFFER,
                                          do_appsink_callback_probe_inp_pad_BUF,
                                          aSaverPtr,
                                          NULL );

    return ( (probe_EOS && probe_BUF) ? appsink_ptr : appsink_ptr );
}


//=======================================================================================
// synopsis: is_ok = do_splicer_negotiate_pads_caps(aPadPtr, aCtsPtr, aQueryPtr)
//
// establishes an acceptable CAPS for linked pads
//=======================================================================================
static gboolean do_splicer_negotiate_pads_caps(GstPad    * aPadPtr,
                                               GstObject * aCtxPtr,
                                               GstQuery  * aQueryPtr)
{
    FlowSplicer_t * splicer_ptr = (FlowSplicer_t *) aCtxPtr;

    gint query_id = GST_QUERY_TYPE(aQueryPtr);

    if (query_id != GST_QUERY_CAPS)
    {
        return gst_pad_query_default(aPadPtr, aCtxPtr, aQueryPtr);
    }

    if (aCtxPtr != (GstObject *) &splicer_ptr)
    {
        return FALSE;
    }

    gboolean  peer_is_from = (aPadPtr == splicer_ptr->ptr_into_pad);

    GstCaps * ptr_all_caps = peer_is_from ? splicer_ptr->ptr_all_from_caps : splicer_ptr->ptr_all_into_caps;

    GstCaps * ptr_query_caps;

    GstCaps * ptr_final_caps;

    gst_query_parse_caps(aQueryPtr, &ptr_query_caps);

    ptr_final_caps = ptr_query_caps;

    if (ptr_all_caps != NULL)
    {
        int idx = gst_caps_get_size(ptr_all_caps);

        // discard sample-rate to allow ANY rate supported by linked elements
        while ( --idx >= 0)
        {
            GstStructure *structure = gst_caps_get_structure(ptr_all_caps, idx);
            gst_structure_remove_field (structure, "rate");
        }

        // the returned CAPS must "intersect" with the pad-template-caps
        GstCaps * ptr_template = gst_pad_get_pad_template_caps(aPadPtr);
        if (ptr_template)
        {
            ptr_final_caps = gst_caps_intersect(ptr_all_caps, ptr_template);
            gst_caps_unref(ptr_template);
        }

        // possibly --- match with the query filter
        if (ptr_query_caps)
        {
            ptr_final_caps = gst_caps_intersect(ptr_all_caps, ptr_query_caps);
            gst_caps_unref (ptr_query_caps);
        }
    }

    if (ptr_final_caps)
    {
        gst_query_set_caps_result (aQueryPtr, ptr_final_caps);
    }

    return TRUE;
}


//=======================================================================================
// synopsis: result = do_callback_for_consumer_INP_pad_events(aPadPtr, aInfoPtr, aCtxPtr)
//
// splicer callback for producer-element-output-pad events
//=======================================================================================
static gboolean do_callback_for_consumer_INP_pad_events(GstPad    * aPadPtr,
                                                        GstObject * aCtxPtr,
                                                        GstEvent  * aEventPtr)
{
    FlowSplicer_t * splicer_ptr = (FlowSplicer_t *) aCtxPtr;

    gboolean is_ok = FALSE;

    gint  event_id = GST_EVENT_TYPE(aEventPtr);

    if (event_id == GST_EVENT_CAPS)
    {
        GstQuery * ptr_query = gst_query_new_caps (splicer_ptr->ptr_all_from_caps);

        is_ok = do_splicer_negotiate_pads_caps(aPadPtr, (GstObject*) splicer_ptr, ptr_query);

        gst_query_unref(ptr_query);
    }
    else
    {
        is_ok = gst_pad_event_default(aPadPtr, aCtxPtr, aEventPtr);
    }

    return is_ok;
}


//=======================================================================================
// synopsis: result = do_splicer_callback_for_consumer_inp_pad(aPadPtr, aInfoPtr, aCtxPtr)
//
// splicer callback for consumer-element-input-pad events
//=======================================================================================
static GstPadProbeReturn do_splicer_callback_for_consumer_inp_pad(GstPad          * aPadPtr,
                                                                  GstPadProbeInfo * aInfoPtr,
                                                                  gpointer          aCtxPtr)
{
    FlowSplicer_t * splicer_ptr = (FlowSplicer_t *) aCtxPtr;

    GstEvent* event_data_ptr = GST_PAD_PROBE_INFO_DATA(aInfoPtr);

    GstEventType  event_type = GST_EVENT_TYPE(event_data_ptr);

    if (event_type != GST_EVENT_EOS)
    {
        return GST_PAD_PROBE_PASS;
    }

    GST_DEBUG_OBJECT(aPadPtr, "the 'consumer-INP-pad' is now EOS !!! ");

    gst_pad_remove_probe(aPadPtr, GST_PAD_PROBE_INFO_ID(aInfoPtr));

    GstPad * ptr_out_pad = gst_element_get_static_pad(splicer_ptr->ptr_into_element,
                                                      splicer_ptr->params.consumer_out_pad_name);

    // possibly --- send EOS onto the consumer OUT pad --- possibly redundant
    if (ptr_out_pad != NULL)
    {
        //? gst_pad_send_event( ptr_out_pad, gst_event_new_eos() );
        gst_object_unref(ptr_out_pad);
    }

    // now we can insert the TEE and change links
    splicer_ptr->status = e_SPLICER_STATE_DONE;

    return GST_PAD_PROBE_DROP;
}


//=======================================================================================
// synopsis: result = do_callback_for_splicer_producer_pad(aPadPtr, aInfoPtr, aCtxPtr)
//
// splicer callback for producer-element-output-pad events
//=======================================================================================
static GstPadProbeReturn do_callback_for_splicer_producer_pad(GstPad          * aPadPtr,
                                                              GstPadProbeInfo * aInfoPtr,
                                                              gpointer          aCtxPtr)
{
    FlowSplicer_t * splicer_ptr = (FlowSplicer_t *) aCtxPtr;

    gulong  probe_info_id = GST_PAD_PROBE_INFO_ID(aInfoPtr);

    GST_DEBUG_OBJECT(aPadPtr, "the 'producer' pad is now blocked !!! ");

    // remove the producer-pad-probe
    gst_pad_remove_probe(aPadPtr, probe_info_id);

    GstPad * ptr_inp_pad = gst_element_get_static_pad(splicer_ptr->ptr_into_element,
                                                      splicer_ptr->params.consumer_inp_pad_name);

    // install consumer-inp-pad-probe for EOS event
    gst_pad_add_probe(ptr_inp_pad,
                      GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
                      do_splicer_callback_for_consumer_inp_pad,
                      aCtxPtr,
                      NULL);

    // send EOS onto the consumer INP pad
    gst_pad_send_event( ptr_inp_pad, gst_event_new_eos() );

    gst_object_unref(ptr_inp_pad);

    return GST_PAD_PROBE_OK;
}


//=======================================================================================
// synopsis: is_ok = do_splicer_unlink_two_elements(aSaverPtr)
//
// safely unlinks two inter-linked elements --- returns TRUE on success, else rrror
//=======================================================================================
static gboolean do_splicer_unlink_two_elements(FramesSaver_t * aSaverPtr)
{
    #define DESIRED_EFFECTS  "videoflip, edgetv"    // TO-DO --- maybe

    static gchar * Options_Effects_Ptr = NULL;

    static GOptionEntry Options_Effects_Array[] =
    {
        { "Options_Effects", 'e', 0, G_OPTION_ARG_STRING, &Options_Effects_Ptr, DESIRED_EFFECTS, NULL },
        { NULL }
    };


    FlowSplicer_t * splicer_ptr = do_get_splicer_ptr( aSaverPtr );

    splicer_ptr->status = e_SPLICER_STATE_FAIL;

    // verify existence of the elements to be spliced by a TEE
    if ( (splicer_ptr->ptr_from_element == NULL) || (splicer_ptr->ptr_into_element == NULL) )
    {
        return FALSE;
    }

    // verify existence of the desired pads
    if ( (splicer_ptr->ptr_from_pad == NULL) || (splicer_ptr->ptr_into_pad == NULL) )
    {
        return FALSE;
    }

    // get desired effects/elements for probes --- TO-DO maybe someday
    if (splicer_ptr->pp_effects_names != NULL)
    {
        GOptionContext * ptr_options_ctx = g_option_context_new("");

        g_option_context_add_main_entries( ptr_options_ctx, Options_Effects_Array, NULL );

        g_option_context_add_group( ptr_options_ctx, gst_init_get_option_group() );

        g_option_context_free( ptr_options_ctx );

        if (Options_Effects_Ptr == NULL)
        {
            Options_Effects_Ptr = DESIRED_EFFECTS;
        }

        splicer_ptr->pp_effects_names = g_strsplit( Options_Effects_Ptr, ",", -1 );
    }

    if (splicer_ptr->pp_effects_names != NULL)
    {
        int index = -1;

        while ( splicer_ptr->pp_effects_names[++index] != NULL )
        {
            const gchar * psz_effect_name = splicer_ptr->pp_effects_names[index];

            GstElement  * ptr_new_effect = gst_element_factory_make(psz_effect_name, NULL);

            if (ptr_new_effect != NULL)
            {
                g_queue_push_tail( &splicer_ptr->effects_queue, ptr_new_effect );

                GST_LOG("Added (%s) to Unlinker-Effects-Queue \n", psz_effect_name);
            }
            else
            {
                GST_LOG("Failed to make Unlinker-Effect (%s) \n", psz_effect_name);
            }
        }
    }

    // start the safe unlinking procedure
    splicer_ptr->status             = e_SPLICER_STATE_BUSY;
    splicer_ptr->ptr_snaps_pipeline = aSaverPtr->parent_pipeline_ptr;

    // get the first queued effect --- possibly none
    splicer_ptr->ptr_current_effect = g_queue_pop_head( &splicer_ptr->effects_queue );

    // apply probe to 'producer' pad --- stop data flow into consumer element
    gst_pad_add_probe(splicer_ptr->ptr_from_pad,
                      GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
                      do_callback_for_splicer_producer_pad,
                      splicer_ptr,
                      NULL);

    return TRUE;   // success --- safe unlinking procedure is now in progress
}


//=======================================================================================
// synopsis: result = do_splicer_configure_TEE_pads()
//
// obtains the internal pads of a TEE --- returns 1 on success, else rrror
//=======================================================================================
static gint do_splicer_configure_TEE_pads(FramesSaver_t * aSaverPtr)
{
    GstPad  * tee_Q1_pad_ptr,
            * tee_Q2_pad_ptr,
            * queue1_pad_ptr,
            * queue2_pad_ptr;

    GstElementClass * class_ptr = GST_ELEMENT_GET_CLASS(aSaverPtr->tee_element_ptr);

    GstPadTemplate  * pad_template_ptr = gst_element_class_get_pad_template(class_ptr, "src_%u");

    if (! pad_template_ptr)
    {
        GST_WARNING("Unable to get pad template");
        return 0;
    }

    tee_Q1_pad_ptr = gst_element_request_pad(aSaverPtr->tee_element_ptr,
                                             pad_template_ptr,
                                             NULL,
                                             NULL);

    queue1_pad_ptr = gst_element_get_static_pad(aSaverPtr->Q_1_element_ptr, "sink");

    tee_Q2_pad_ptr = gst_element_request_pad(aSaverPtr->tee_element_ptr,
                                             pad_template_ptr,
                                             NULL,
                                             NULL);

    queue2_pad_ptr = gst_element_get_static_pad(aSaverPtr->Q_2_element_ptr, "sink");

    if (gst_pad_link(tee_Q1_pad_ptr, queue1_pad_ptr) != GST_PAD_LINK_OK)
    {
        GST_WARNING("Tee for q1 could not be linked.\n");
        return 0;
    }

    if (gst_pad_link(tee_Q2_pad_ptr, queue2_pad_ptr) != GST_PAD_LINK_OK)
    {
        GST_WARNING("Tee for q2 could not be linked.\n");
        return 0;
    }

    gst_object_unref(queue1_pad_ptr);
    gst_object_unref(queue2_pad_ptr);

    return 1;
}


//=======================================================================================
// synopsis: result = do_frame_saver_element_cleanup()
//
// inserts TEE into the pipeline --- returns 0 on always
//=======================================================================================
static gint do_frame_saver_element_cleanup(FramesSaver_t * aSaverPtr)
{
    if (The_MainLoop_Ptr != NULL)
    {
        g_main_loop_quit(The_MainLoop_Ptr);

        gst_object_unref(GST_OBJECT(aSaverPtr->parent_pipeline_ptr));

        g_main_loop_unref(The_MainLoop_Ptr);

        aSaverPtr->parent_pipeline_ptr = NULL;

        The_MainLoop_Ptr = NULL;
    }

    if (aSaverPtr->parent_pipeline_ptr != NULL)
    {
        gst_object_unref(aSaverPtr->parent_pipeline_ptr);

        aSaverPtr->parent_pipeline_ptr = NULL;
    }

    return 0;
}


//=======================================================================================
// synopsis: is_ok = do_pipeline_callback_for_bus_messages()
//
// handle pipeline's bus messages --- returns TRUE on success, else rrror
//=======================================================================================
static gboolean do_pipeline_callback_for_bus_messages(GstBus     * aBusPtr,
                                                      GstMessage * aMsgPtr,
                                                      gpointer     aCtxPtr)
{
    const GstStructure * gst_struct_ptr;

    GError    * ptr_err = NULL;
    gchar     * ptr_dbg = NULL;

    FramesSaver_t  * saver_ptr = (FramesSaver_t *) aCtxPtr;

    int saver_ID = saver_ptr ? saver_ptr->instance_ID : 0;

    int msg_type = GST_MESSAGE_TYPE(aMsgPtr);

#ifdef _NO_BUS_TRACE
	if (saver_ID >= 0)	// condition is always TRUE
	{
		return FALSE;	// FALSE result prevents being called again
	}
#endif

    switch (msg_type)
    {
    case GST_MESSAGE_ERROR:
        gst_message_parse_error(aMsgPtr, &ptr_err, &ptr_dbg);
        GST_ERROR(PREFIX_FORMAT "bus_msg=ERROR --- %s", saver_ID, ptr_err->message);
        gst_object_default_error(aMsgPtr->src, ptr_err, ptr_dbg);
        g_clear_error(&ptr_err);
        g_error_free(ptr_err);
        g_free(ptr_dbg);
        g_main_loop_quit(The_MainLoop_Ptr);
        break;

    case GST_MESSAGE_APPLICATION:
        gst_struct_ptr = gst_message_get_structure(aMsgPtr);
        if (gst_structure_has_name(gst_struct_ptr, "turn_off"))
        {
        	GST_ERROR(PREFIX_FORMAT "bus_msg=OFF \n", saver_ID);
            g_main_loop_quit(The_MainLoop_Ptr);
        }
        break;

    case GST_MESSAGE_EOS:
    	GST_ERROR(PREFIX_FORMAT "bus_msg=EOS \n", saver_ID);
        g_main_loop_quit(The_MainLoop_Ptr);
        break;

    default:
        break;
    }

    if (msg_type == GST_MESSAGE_STATE_CHANGED)
    {
        GstState old_state, new_state, pending_state;

        gst_message_parse_state_changed(aMsgPtr, &old_state, &new_state, &pending_state);

        GST_LOG(PREFIX_FORMAT "bus_msg=STATE --- old=%i  new=%i  pending=%i. \n", saver_ID, old_state, new_state, pending_state);
    }
    else
    {
        const char * psz_msg_type_name = GST_MESSAGE_TYPE_NAME(aMsgPtr);

        int msg_time = (int) aMsgPtr->timestamp;

        GST_LOG(PREFIX_FORMAT "bus_msg=OTHER --- name=(%s) time=%i  type=%i  \n", saver_ID, psz_msg_type_name, msg_time, msg_type);
    }

    return TRUE;
}


//=======================================================================================
// synopsis: result = do_pipeline_insert_TEE_splicer()
//
// inserts TEE into the pipeline --- returns 0 on done, 1 on busy, else error
//=======================================================================================
static e_TEE_INSERT_STATE_e  do_pipeline_insert_TEE_splicer(FramesSaver_t * aSaverPtr)
{
    FlowSplicer_t * splicer_ptr = do_get_splicer_ptr( aSaverPtr );

    GstState pipeline_state = GST_STATE_NULL;

    gboolean is_TEE_in_pipe = (gst_element_get_parent(aSaverPtr->tee_element_ptr) != NULL);

    gboolean   is_linked_ok = FALSE;

    // possibly --- our TEE is already in the pipeline --- nothing more to do
    if (is_TEE_in_pipe)
    {
        return e_TEE_INSERT_ENDED;
    }

    gst_element_get_state(aSaverPtr->parent_pipeline_ptr, &pipeline_state, NULL, 0);

    // dynamic unlinking is needed when the pipeline is PLAYING
    if (pipeline_state != GST_STATE_NULL)
    {
        // possibly --- start new unlinking procedure
        if ((splicer_ptr->status == e_SPLICER_STATE_NONE) ||
            (splicer_ptr->status == e_SPLICER_STATE_USED) )
        {
            do_splicer_unlink_two_elements( aSaverPtr );   // affects "splicer_ptr->status"
        }

        // possibly --- failed to unlink elements
        if (splicer_ptr->status == e_SPLICER_STATE_FAIL)
        {
            return e_TEE_INSERT_ERROR;
        }

        // possibly --- wait for end of unlinking procedure
        if (splicer_ptr->status == e_SPLICER_STATE_BUSY)
        {
            return e_TEE_INSERT_WAITS;
        }

        // suspend the producer and consumer elements
        GstStateChangeReturn rv1 = gst_element_set_state(splicer_ptr->ptr_from_element,  GST_STATE_PAUSED);
        GstStateChangeReturn rv2 = gst_element_set_state(splicer_ptr->ptr_into_element,  GST_STATE_PAUSED);
        GstStateChangeReturn rv3 = gst_element_set_state(aSaverPtr->parent_pipeline_ptr, GST_STATE_PAUSED);

        // possibly --- the unlinking procedure failed
        if (splicer_ptr->status != e_SPLICER_STATE_DONE)
        {
            return e_TEE_INSERT_ERROR + ((rv1 && rv2 && rv3) ? 0 : 0);  // rv's only for Debug
        }
    }

    if (pipeline_state != GST_STATE_NULL)
    {
        if (! gst_pad_unlink(splicer_ptr->ptr_from_pad, splicer_ptr->ptr_into_pad))
        {
            g_warning("Unable to unlink pads of connected pipeline elements \n");
            return e_FAIL_UNLINK_PADS;
        }
    }

    // now we can safely unlink the pipe where the TEE must be inserted
    gst_element_unlink(splicer_ptr->ptr_from_element, splicer_ptr->ptr_into_element);

    // add necessary splicing elements to pipeline
    gst_bin_add_many(GST_BIN(aSaverPtr->parent_pipeline_ptr),
                     aSaverPtr->tee_element_ptr,
                     aSaverPtr->Q_1_element_ptr,
                     aSaverPtr->Q_2_element_ptr,
                     aSaverPtr->cvt_element_ptr,
                     aSaverPtr->video_sink2_ptr,
                     NULL);

    if (do_splicer_configure_TEE_pads(aSaverPtr) != 1)
    {
        return e_FAIL_TEE_IN_PADS;
    }

    // link elements pads: PRODUCER.OUT pad to TEE.INP pad
    if ( TRUE != gst_element_link_pads(splicer_ptr->ptr_from_element,
                                       splicer_ptr->params.producer_out_pad_name,
                                       aSaverPtr->tee_element_ptr,
                                       "sink") )
    {
        g_warning("Unable to link pads: PRODUCER.OUT --> TEE.SINK \n");
        return e_FAIL_TEE_UP_LINK;
    }

    // get the producer's caps
    GstCaps * ptr_linker_caps = gst_pad_query_caps(splicer_ptr->ptr_from_pad,
                                                   splicer_ptr->ptr_all_from_caps);

    // link elements: T-QUE-1 to CONSUMER
    if ( ptr_linker_caps != NULL )
    {
        is_linked_ok = gst_element_link_filtered(aSaverPtr->Q_1_element_ptr,
                                                 splicer_ptr->ptr_into_element,
                                                 ptr_linker_caps);
        if (! is_linked_ok)
        {
            g_warning("Unable to link elements: T-QUE-1 --> CONSUMER (with producer caps) \n");
            return e_FAIL_TEE_Q1_LINK;
        }
    }
    else    // link T-QUE-1 using negotiated caps when producer caps is null
    {
        gst_pad_set_event_function(splicer_ptr->ptr_into_pad, do_callback_for_consumer_INP_pad_events);

        g_warning("linking pads with negotiated CAPS: T-QUE-1.OUT --> CONSUMER.INP \n");

        is_linked_ok = gst_element_link_pads(aSaverPtr->Q_1_element_ptr,
                                             "src",
                                             splicer_ptr->ptr_into_element,
                                             splicer_ptr->params.consumer_inp_pad_name);
        if (! is_linked_ok)
        {
            g_warning("Unable to link pads: T-QUE-1.OUT --> CONSUMER.INP \n");
            return e_FAIL_TEE_Q1_LINK;
        }
    }

#ifndef _NOT_USING_SINKER_CAPS_
    ptr_linker_caps = aSaverPtr->sinker_caps_ptr;
#endif

    if (ptr_linker_caps == NULL)
    {
        ptr_linker_caps = aSaverPtr->sinker_caps_ptr;
    }

    // link elements: T-QUE-2 to SINK2
    is_linked_ok = gst_element_link_many(aSaverPtr->Q_2_element_ptr,
                                         aSaverPtr->cvt_element_ptr,
                                         NULL);

    is_linked_ok &= gst_element_link_filtered(aSaverPtr->cvt_element_ptr,
                                              aSaverPtr->video_sink2_ptr,
                                              ptr_linker_caps);

    if (! is_linked_ok)
    {
        g_warning("Unable to link elements: T-QUE-2 --> SINK2 \n");
        return e_FAIL_TEE_Q2_LINK;
    }

    return e_TEE_INSERT_ENDED;
}


//=======================================================================================
// synopsis: is_ok = do_pipeline_validate_splicer_parameters()
//
// runs the pipeline --- return TRUE on success, else error
//=======================================================================================
gboolean do_pipeline_validate_splicer_parameters(FramesSaver_t * aSaverPtr)
{
    FlowSplicer_t * splicer_ptr = do_get_splicer_ptr( aSaverPtr );

    do_DBG_print("do_pipeline_validate_splicer_parameters \n", aSaverPtr);

    splicer_ptr->ptr_from_element = gst_bin_get_by_name(GST_BIN(aSaverPtr->parent_pipeline_ptr), splicer_ptr->params.producer_name);

    if ( splicer_ptr->ptr_from_element == NULL )
    {
        g_warning("pipeline missing PRODUCER element (%s) \n", splicer_ptr->params.producer_name);
        return FALSE;
    }

    splicer_ptr->ptr_into_element = gst_bin_get_by_name(GST_BIN(aSaverPtr->parent_pipeline_ptr),
                                                     splicer_ptr->params.consumer_name);

    if ( splicer_ptr->ptr_into_element == NULL )
    {
        g_warning("pipeline missing CONSUMER element (%s) \n", splicer_ptr->params.consumer_name);
        return FALSE;
    }

    // a source pad produces data consumed by a sink pad of the next downstream element
    splicer_ptr->ptr_from_pad = gst_element_get_static_pad(splicer_ptr->ptr_from_element,
                                                        splicer_ptr->params.producer_out_pad_name);

    if ( splicer_ptr->ptr_from_pad == NULL )
    {
        g_warning("pipeline missing PRODUCER output pad (%s) \n", splicer_ptr->params.producer_out_pad_name);
        return FALSE;
    }

    splicer_ptr->ptr_into_pad = gst_element_get_static_pad(splicer_ptr->ptr_into_element,
                                                        splicer_ptr->params.consumer_inp_pad_name);

    if ( splicer_ptr->ptr_into_pad == NULL )
    {
        g_warning("pipeline missing CONSUMER input pad (%s) \n", splicer_ptr->params.consumer_inp_pad_name);
        return FALSE;
    }

    splicer_ptr->ptr_all_from_caps = gst_pad_get_allowed_caps(splicer_ptr->ptr_from_pad);

    splicer_ptr->ptr_all_into_caps = gst_pad_get_allowed_caps(splicer_ptr->ptr_into_pad);

    return TRUE;
}


//=======================================================================================
// synopsis: is_ok = do_pipeline_callback_for_idle_time()
//
// callback when the pipeline's main-loop-thread is idle --- return TRUE always
//=======================================================================================
static gboolean do_pipeline_callback_for_idle_time(gpointer aCtxPtr)
{
    FramesSaver_t   * saver_ptr = (FramesSaver_t *) aCtxPtr;

    FlowSplicer_t * splicer_ptr = do_get_splicer_ptr( saver_ptr );

    GstClockTime  now_nanos = gst_clock_get_time( The_SysClock_Ptr );

    GstClockTime elapsed_ns = now_nanos - The_LaunchTime_ns;

    uint32_t    playtime_ms = (uint32_t) (elapsed_ns / NANOS_PER_MILLISEC);


    // possibly --- TEE was inserted or is not wanted
    if ( (saver_ptr->wait_state_ends_ns == 0) || (saver_ptr->tee_element_ptr == NULL) )
    {
        if (elapsed_ns > saver_ptr->frame_snap_wait_ns)
        {
            gboolean more_ok = do_appsink_trigger_next_frame_snap( saver_ptr, playtime_ms );

            // possibly --- disable snaps --- effectively "infinit" wait time
            if (! more_ok)
            {
                saver_ptr->num_snap_signals = saver_ptr->num_saved_frames;

                saver_ptr->frame_snap_wait_ns += INFINIT_NANOS;
            }
        }

        return TRUE;
    }

    // possibly --- try-or-retry to insert TEE into pipeline
    if ( saver_ptr->wait_state_ends_ns < now_nanos )
    {
        if ((splicer_ptr->status == e_SPLICER_STATE_NONE) || (splicer_ptr->status == e_SPLICER_STATE_USED) )
        {
            GST_LOG(PREFIX_FORMAT "playtime=%u %s", saver_ptr->instance_ID, playtime_ms, "... Starting TEE insertion \n");
        }

        e_TEE_INSERT_STATE_e  status = do_pipeline_insert_TEE_splicer( saver_ptr );

        if (status == e_TEE_INSERT_ENDED)    // TEE inserted into pipeline
        {
            saver_ptr->wait_state_ends_ns = 0;

            gst_element_set_state( saver_ptr->parent_pipeline_ptr, GST_STATE_PLAYING );
            gst_element_set_state( saver_ptr->video_sink2_ptr,     GST_STATE_PLAYING );
            gst_element_set_state( splicer_ptr->ptr_from_element,  GST_STATE_PLAYING );
            gst_element_set_state( splicer_ptr->ptr_into_element,  GST_STATE_PLAYING );

            GST_LOG(PREFIX_FORMAT "playtime=%u %s", saver_ptr->instance_ID, playtime_ms, "... Finished TEE insertion\n");
        }
        else if (status != e_TEE_INSERT_WAITS)    // failed --- Retry later
        {
            saver_ptr->wait_state_ends_ns = now_nanos + (NANOS_PER_MILLISEC * splicer_ptr->params.one_tick_ms * 10);

            GST_LOG(PREFIX_FORMAT "playtime=%u %s", saver_ptr->instance_ID, playtime_ms, "... Failed TEE insertion\n");

            gst_element_set_state( saver_ptr->parent_pipeline_ptr, GST_STATE_READY );
        }
        else // NOTE: here we could report the typical intervals between "idle" callbacks
        {

        }

        return TRUE;
    }

    return TRUE;
}


//=======================================================================================
// synopsis: is_ok = do_pipeline_callback_for_timer_tick()
//
// callback for the pipeline's timer-tick event --- return TRUE always
//=======================================================================================
static gboolean do_pipeline_callback_for_timer_tick(gpointer aCtxPtr)
{
    FramesSaver_t   * saver_ptr = (FramesSaver_t *) aCtxPtr;

    FlowSplicer_t * splicer_ptr = do_get_splicer_ptr( saver_ptr );

    GstClockTime  now_nanos = gst_clock_get_time( The_SysClock_Ptr );

    GstClockTime elapsed_ns = now_nanos - The_LaunchTime_ns;

    uint32_t    playtime_ms = (uint32_t) (elapsed_ns / NANOS_PER_MILLISEC);


    if (now_nanos < saver_ptr->wait_state_ends_ns)   // idle waiting state
    {
        GST_DEBUG(PREFIX_FORMAT "playtime=%u ... idle-wait \n", saver_ptr->instance_ID, playtime_ms);
    }
    else if (splicer_ptr->params.one_snap_ms == 0)     // not doing frame snaps
    {
        GST_DEBUG(PREFIX_FORMAT "playtime=%u \n", saver_ptr->instance_ID, playtime_ms);
    }

    // possibly --- it's time to shutdown the pipeline being tested
    if ( playtime_ms > splicer_ptr->params.max_play_ms )
    {
        GST_DEBUG(PREFIX_FORMAT "playtime=%u ... play-ends \n", saver_ptr->instance_ID, playtime_ms);

        if (gst_element_get_parent(saver_ptr->vid_sourcer_ptr) != NULL)  // default pipeline
        {
            gst_element_send_event ( saver_ptr->vid_sourcer_ptr, gst_event_new_eos() );

            gst_element_set_state( saver_ptr->vid_sourcer_ptr, GST_STATE_READY );
        }

        gst_element_set_state( saver_ptr->parent_pipeline_ptr, GST_STATE_NULL );

        do_frame_saver_element_cleanup( saver_ptr );

        return FALSE;
    }

    return TRUE;
}


//=======================================================================================
// synopsis: result = do_frame_saver_create_splicer_elements(aSaverPtr)
//
// creates the pipeline's spicer elements --- return 0 on success, else error
//=======================================================================================
static PIPELINE_MAKER_ERROR_e do_frame_saver_create_splicer_elements(FramesSaver_t * aSaverPtr)
{
    FlowSplicer_t * splicer_ptr = do_get_splicer_ptr( aSaverPtr );

    do_DBG_print("do_frame_saver_create_splicer_elements \n", aSaverPtr);

    if (do_pipeline_validate_splicer_parameters(aSaverPtr) != TRUE)
    {
        return e_ERROR_PIPELINE_TEE_PARAMS;
    }

    // create the splicer's elements
    aSaverPtr->tee_element_ptr = gst_element_factory_make("tee",          "FSL_TEE");
    aSaverPtr->Q_1_element_ptr = gst_element_factory_make("queue",        "FSL_T_QUEUE_1");
    aSaverPtr->Q_2_element_ptr = gst_element_factory_make("queue",        "FSL_T_QUEUE_2");
    aSaverPtr->cvt_element_ptr = gst_element_factory_make("videoconvert", "FSL_T_VID_CVT");

    // possibly --- create the appsink to snap and save frames
    if (splicer_ptr->params.one_snap_ms > 0)
    {
        aSaverPtr->sinker_caps_ptr = gst_caps_from_string(CAPS_FOR_SNAP_SINKER);

        aSaverPtr->video_sink2_ptr = do_appsink_create( aSaverPtr, "FSL_T_APP_SINK", NUM_APP_SINK_BUFFERS );
    }
    else
    {
        aSaverPtr->sinker_caps_ptr = gst_caps_from_string(CAPS_FOR_VIEW_SINKER);

        aSaverPtr->video_sink2_ptr = gst_element_factory_make("autovideosink", "FSL_T_VID_SINK");
    }

    int flags = (aSaverPtr->tee_element_ptr ? 0 : 0x0010) +
                (aSaverPtr->Q_1_element_ptr ? 0 : 0x0020) +
                (aSaverPtr->Q_2_element_ptr ? 0 : 0x0040) +
                (aSaverPtr->cvt_element_ptr ? 0 : 0x0100) +
                (aSaverPtr->video_sink2_ptr ? 0 : 0x0200) +
                (aSaverPtr->sinker_caps_ptr ? 0 : 0x0400) ;

    if (flags != 0)
    {
        g_warning("Failed creating some elements for splicing (flags=0x%0X) \n", flags);
        return e_FAILED_MAKE_WORK_ELEMENTS;
    }

    return e_PIPELINE_MAKER_ERROR_NONE;
}


//=======================================================================================
// synopsis: result = do_pipeline_create_default_elements(aSaverPtr)
//
// creates elements for default pipeline --- return 0 on success, else error
//=======================================================================================
static PIPELINE_MAKER_ERROR_e do_pipeline_create_default_elements(FramesSaver_t * aSaverPtr)
{
    do_DBG_print("do_pipeline_create_default_elements \n", aSaverPtr);

    // create the elements of the default pipeline
    aSaverPtr->source_caps_ptr = gst_caps_from_string(CAPS_FOR_AUTO_SOURCE);
    aSaverPtr->vid_sourcer_ptr = gst_element_factory_make("videotestsrc",  DEFAULT_VID_SRC_NAME);
    aSaverPtr->vid_convert_ptr = gst_element_factory_make("videoconvert",  DEFAULT_VID_CVT_NAME);
    aSaverPtr->video_sink1_ptr = gst_element_factory_make("autovideosink", "auto_video_sink_0");

    int flags = (aSaverPtr->vid_sourcer_ptr ? 0 : 0x001) +   // only used for default pipeline
                (aSaverPtr->video_sink1_ptr ? 0 : 0x002) +   // only used for default pipeline
                (aSaverPtr->vid_convert_ptr ? 0 : 0x004) +
                (aSaverPtr->source_caps_ptr ? 0 : 0x008) ;

    if (flags != 0)
    {
        g_warning("Failed creating basic pipeline elements (flags=0x%0X) \n", flags);
        return e_FAILED_MAKE_WORK_ELEMENTS;
    }

    g_object_set(G_OBJECT(aSaverPtr->vid_sourcer_ptr), "is-live", TRUE, NULL);

    gst_bin_add_many(GST_BIN(aSaverPtr->parent_pipeline_ptr),
                     aSaverPtr->vid_sourcer_ptr ,
                     aSaverPtr->vid_convert_ptr,
                     aSaverPtr->video_sink1_ptr,
                     NULL);

    // link elements: SRC to CVT
    gboolean is_link_1_ok = gst_element_link_many(aSaverPtr->vid_sourcer_ptr,
                                                  aSaverPtr->vid_convert_ptr,
                                                  NULL);

    // link elements: CVT with CAPS to SINK1
    gboolean is_link_2_ok = gst_element_link_filtered(aSaverPtr->vid_convert_ptr,
                                                      aSaverPtr->video_sink1_ptr,
                                                      aSaverPtr->source_caps_ptr);

    if ( (! is_link_1_ok) || (! is_link_2_ok) )
    {
        g_warning("Failed linking basic elements: SRC --> CVT --> CAPS --> SINK1 \n");
        return e_FAILED_LINK_MINI_PIPELINE;
    }

    return e_PIPELINE_MAKER_ERROR_NONE;
}


//=======================================================================================
// synopsis: result = do_pipeline_create_instance()
//
// creates the pipeline --- return 0 on success, else error
//=======================================================================================
static PIPELINE_MAKER_ERROR_e do_pipeline_create_instance(FramesSaver_t * aSaverPtr)
{
    FlowSplicer_t * splicer_ptr = do_get_splicer_ptr( aSaverPtr );

    do_DBG_print("do_pipeline_create_instance \n", aSaverPtr);

    PIPELINE_MAKER_ERROR_e e_result = e_PIPELINE_MAKER_ERROR_NONE;

    gboolean is_custom = (strchr(splicer_ptr->params.pipeline_spec, '!') != NULL);

    if ( is_custom )
    {
        GError * error_ptr = NULL;

        aSaverPtr->parent_pipeline_ptr = gst_parse_launch(splicer_ptr->params.pipeline_spec, &error_ptr);

        if (error_ptr != NULL)
        {
            g_warning("Failed creating custom pipeline --- (%s) \n", error_ptr->message);
            return e_PIPELINE_PARSER_HAS_ERROR;
        }

        if (gst_object_set_name(GST_OBJECT(aSaverPtr->parent_pipeline_ptr), splicer_ptr->params.pipeline_name) != TRUE)
        {
            g_warning("Failed naming custom pipeline (%s) \n", splicer_ptr->params.pipeline_name);
            return e_FAILED_MAKE_PIPELINE_NAME;
        }
    }
    else
    {
        aSaverPtr->parent_pipeline_ptr = gst_pipeline_new(splicer_ptr->params.pipeline_name);

        if (! aSaverPtr->parent_pipeline_ptr)
        {
            g_warning("Failed creating pipeline named (%s) \n", splicer_ptr->params.pipeline_name);
            return e_FAILED_MAKE_MINI_PIPELINE;
        }
    }

    if (gst_element_set_state(aSaverPtr->parent_pipeline_ptr, GST_STATE_NULL) != GST_STATE_CHANGE_SUCCESS)
    {
        g_warning("Failed setting new pipeline state to NULL \n");
        return e_FAILED_SET_PIPELINE_STATE;
    }

    aSaverPtr->bus_ptr = gst_pipeline_get_bus(GST_PIPELINE(aSaverPtr->parent_pipeline_ptr));

    if (! aSaverPtr->bus_ptr)
    {
        g_warning("Failed obtaining the pipeline's bus \n");
        return e_FAILED_FETCH_PIPELINE_BUS;
    }

    gst_bus_add_watch(aSaverPtr->bus_ptr, do_pipeline_callback_for_bus_messages, NULL);
    gst_object_unref(aSaverPtr->bus_ptr);
    aSaverPtr->bus_ptr = NULL;

    // possibly --- configure the default pipeline
    if (! is_custom)
    {
        e_result = do_pipeline_create_default_elements( aSaverPtr );
    }

    if (e_result == e_PIPELINE_MAKER_ERROR_NONE)
    {
        e_result = do_frame_saver_create_splicer_elements( aSaverPtr );
    }

    return e_result;
}


//=======================================================================================
// synopsis: is_ok = do_prepare_to_play( aSaverPtr, canSplicePipeline )
//
// prepares to run the main loop the pipeline --- return TRUE on success, else error
//=======================================================================================
static gboolean do_prepare_to_play( FramesSaver_t * aSaverPtr, gboolean canSplicePipeline )
{
    FlowSplicer_t * splicer_ptr = do_get_splicer_ptr( aSaverPtr );

    GstClockTime some_nanos = (NANOS_PER_MILLISEC * MIN_TICKS_MILLISEC) / 10;
    GstClockTime play_nanos = (NANOS_PER_MILLISEC * splicer_ptr->params.max_play_ms) + some_nanos;
    GstClockTime wait_nanos = (NANOS_PER_MILLISEC * splicer_ptr->params.max_wait_ms) + some_nanos;
    GstClockTime  now_nanos = gst_clock_get_time( The_SysClock_Ptr );

    do_DBG_print("do_prepare_to_play \n", aSaverPtr);

    strcpy(aSaverPtr->work_folder_path, splicer_ptr->params.folder_path);

    aSaverPtr->wait_state_ends_ns = (splicer_ptr->params.max_wait_ms < 1) ? 0 : wait_nanos + now_nanos;
    aSaverPtr->frame_snap_wait_ns = (splicer_ptr->params.one_snap_ms > 0) ? 0 : play_nanos + INFINIT_NANOS;

    aSaverPtr->num_stream_frames = 0;
    aSaverPtr->num_stream_errors = 0;

    aSaverPtr->num_saver_errors = 0;
    aSaverPtr->num_saved_frames = 0;
    aSaverPtr->num_snap_signals = 0;

    if ( canSplicePipeline )
    {
        if (aSaverPtr->tee_element_ptr == NULL)
        {
            do_frame_saver_create_splicer_elements( aSaverPtr );
        }
    }

    g_idle_add( do_pipeline_callback_for_idle_time, aSaverPtr );

    return TRUE;
}


//=======================================================================================
// synopsis: is_ok = do_pipeline_test_run_main_loop(aSaverPtr)
//
// runs the main-loop and plays the pipeline --- return TRUE on success, else error
//=======================================================================================
static gboolean do_pipeline_test_run_main_loop(FramesSaver_t * aSaverPtr)
{
    FlowSplicer_t * splicer_ptr = do_get_splicer_ptr( aSaverPtr );

    do_prepare_to_play( aSaverPtr, TRUE );

    // possibly --- insert the TEE now, before pipeline starts playing
    if (splicer_ptr->params.max_wait_ms <= 0)
    {
        if (do_pipeline_insert_TEE_splicer(aSaverPtr) != e_TEE_INSERT_ENDED)
        {
            return FALSE;
        }
    }

    do_DBG_print("do_pipeline_test_run_main_loop \n", aSaverPtr);

    The_MainLoop_Ptr = g_main_loop_new(NULL, FALSE);

    if (gst_element_set_state(aSaverPtr->parent_pipeline_ptr, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
        g_warning("Failed setting pipeline state to PLAYING \n");
        return FALSE;
    }

    GST_LOG(PREFIX_FORMAT "PLAYING \n", aSaverPtr->instance_ID);

    g_timeout_add(splicer_ptr->params.one_tick_ms, do_pipeline_callback_for_timer_tick, aSaverPtr);

    g_main_loop_run(The_MainLoop_Ptr);

    GST_LOG(PREFIX_FORMAT "EXITING \n", aSaverPtr->instance_ID);

    return TRUE;
}


//=======================================================================================
// synopsis: result = Frame_Saver_Filter_Attach(aPluginPtr)
//
// should be called when plugin is created --- returns 0 on success, else error
//=======================================================================================
int Frame_Saver_Filter_Attach(GstElement * aPluginPtr)
{
    int index = do_find_plugin_index(aPluginPtr);     // -1 if not found

    // verify validity of plugin element
    if (aPluginPtr == NULL)
    {
        do_DBG_print("Attach_GST --- ERROR ---- PluginPtr is NULL \n", NULL);
        return -1;
    }

    FramesSaver_t * a_saver_ptr = &The_FramesSavers_Array[index];

    // possibly --- plugin is known --- allowed
    if (index >= 0)
    {
        do_DBG_print("Attach_GST --- KNOWN \n", a_saver_ptr);
        return 0;
    }

    if (! The_Mutex_Handle)
    {
        do_DBG_print("Attach_GST --- ERROR ---- The_Mutex_Handle \n", a_saver_ptr);
        return -2;  // mutex does not exist
    }

    if (nativeTryLockMutex(The_Mutex_Handle, MUTEX_TIMEOUT_MS) != 0)
    {
        do_DBG_print("Attach_GST --- ERROR ---- MUTEX_TIMEOUT_MS \n", a_saver_ptr);
        return -3;  // failed to acquire mutex
    }

    // establish the starting index for finding the first unused slot
    index = (The_Plugins_Count < MAX_NUM_PLUGINS) ? -1 : MAX_NUM_PLUGINS;

    // find the first unused slot in the array
    while ( (++index < MAX_NUM_PLUGINS) && (The_FramesSavers_Array[index].instance_ID > 0) )
    {
        ; // next
    }

    FramesSaver_t *   saver_ptr = &The_FramesSavers_Array[index];

    FlowSplicer_t * splicer_ptr = do_get_splicer_ptr(saver_ptr);

    if ( index >= MAX_NUM_PLUGINS )
    {
        do_DBG_print("Attach_GST --- ERROR --- REACHED-LIMIT \n", saver_ptr);
    }
    else if ( saver_ptr->attached_plugin_ptr != NULL )
    {
        do_DBG_print("Attach_GST --- ERROR --- ENTRY-IS-USED \n", saver_ptr);
    }
    else    // mark the slot as being used --- TO-DO: find-pipeline-as-root-parent
    {
        ++The_Plugins_Count;

        saver_ptr->attached_plugin_ptr = aPluginPtr;

        saver_ptr->parent_pipeline_ptr = (GstElement *) gst_element_get_parent(aPluginPtr);

        saver_ptr->instance_ID = index + 1;

        frame_saver_params_initialize( &splicer_ptr->params );

        do_DBG_print("Attach_GST --- SUCCESS \n", saver_ptr);
    }

    nativeReleaseMutex(The_Mutex_Handle);

    return (saver_ptr->attached_plugin_ptr == aPluginPtr) ? 0 : -1;
}


//=======================================================================================
// synopsis: result = Frame_Saver_Filter_Detach(aPluginPtr)
//
// should be called when plugin gets EOS --- returns 0 on success, else error
//=======================================================================================
int Frame_Saver_Filter_Detach(GstElement * aPluginPtr)
{
    if (! The_Mutex_Handle)
    {
        do_DBG_print("Detach_GST --- ERROR ---- The_Mutex_Handle \n", NULL);
        return -2;  // mutex does not exist
    }

    if (nativeTryLockMutex(The_Mutex_Handle, MUTEX_TIMEOUT_MS) != 0)
    {
        do_DBG_print("Detach_GST --- ERROR ---- MUTEX_TIMEOUT_MS \n", NULL);
        return -3;  // failed to acquire mutex
    }

    int index = do_find_plugin_index(aPluginPtr);     // -1 if not found

    // possibly --- plugin is unknown
    if (index < 0)
    {
        nativeReleaseMutex(The_Mutex_Handle);

        do_DBG_print("Detach_GST --- UNKNOWN \n", NULL);

        return -1;
    }

    FramesSaver_t * saver_ptr = &The_FramesSavers_Array[index];

    // mark the slot as empty and unused
    saver_ptr->attached_plugin_ptr = NULL;
    saver_ptr->parent_pipeline_ptr = NULL;
    saver_ptr->instance_ID         = 0;

    do_DBG_print("Detach_GST --- SUCCESS \n", saver_ptr);

    // release and/or delete mutex
    if ( --The_Plugins_Count == 0 )
    {
        void * ptr_mutex = The_Mutex_Handle;
        The_Mutex_Handle = NULL;
        nativeDeleteMutex(ptr_mutex);
    }
    else
    {
        nativeReleaseMutex(The_Mutex_Handle);
    }

    return 0;
}


//=======================================================================================
// synopsis: result = Frame_Saver_Filter_Receive_Buffer(aPluginPtr, aBufferPtr, aCapsTextPtr)
//
// called at by the actual plugin upon buffer arrival --- returns GST_FLOW_OK
//=======================================================================================
int Frame_Saver_Filter_Receive_Buffer(GstElement * aPluginPtr,
                                      GstBuffer  * aBufferPtr,
                                      const char * aCapsTextPtr)
{
    int result = (int) GST_FLOW_ERROR;

    int  index = do_find_plugin_index(aPluginPtr);     // -1 if not found

    if (index < 0)
    {
        return result;
    }

    FramesSaver_t *   saver_ptr = &The_FramesSavers_Array[index];

    FlowSplicer_t * splicer_ptr = do_get_splicer_ptr(saver_ptr);

    saver_ptr->num_stream_frames += 1;

    if (splicer_ptr->params.one_snap_ms > 0)
    {
        if (saver_ptr->num_snap_signals > saver_ptr->num_saved_frames)
        {
            result = do_save_frame_buffer( (GstBuffer *) aBufferPtr, aCapsTextPtr, saver_ptr );
        }
    }

    return result;
}


//=======================================================================================
// synopsis: result = Frame_Saver_Filter_Transition(aPluginPtr)
//
// called at by the actual plugin upon state change --- returns 0 on success, else error
//=======================================================================================
int Frame_Saver_Filter_Transition(GstElement * aPluginPtr, GstStateChange aTransition)
{
    int index = do_find_plugin_index(aPluginPtr);     // -1 if not found

    if (index >= 0)
    {
        FramesSaver_t *   saver_ptr = &The_FramesSavers_Array[index];

        FlowSplicer_t * splicer_ptr = do_get_splicer_ptr(saver_ptr);

        do_DBG_print("Transition \n", saver_ptr);

        if (aTransition == GST_STATE_CHANGE_NULL_TO_READY)
        {
            char report[MAX_PARAMS_SPECS_LNG];

            frame_saver_params_write_to_buffer( &splicer_ptr->params, report, sizeof(report) );

            do_prepare_to_play( saver_ptr, FALSE );

            GST_LOG(report);
        }
    }

    return 0;
}


//=======================================================================================
// synopsis: result = Frame_Saver_Filter_Set_Params(aPluginPtr, aNewValuePtr, aDstValuePtr)
//
// called at by the actual plugin to change params --- returns 0 on success, else error
//=======================================================================================
int Frame_Saver_Filter_Set_Params(GstElement  * aPluginPtr,
                                  const gchar * aNewValuePtr,
                                  gchar       * aDstValuePtr)
{
    const char * psz_note = "";

    char params_specs[MAX_PARAMS_SPECS_LNG];

    int index = do_find_plugin_index(aPluginPtr);     // -1 if not found

    int error = 0;

    // possibly --- plugin is unknown
    if (index < 0)
    {
        return -1;
    }

    // possibly --- invalid length
    if ( sprintf(params_specs, "%s", aNewValuePtr) != (int) strlen(aNewValuePtr) )
    {
        return -2;
    }

    FramesSaver_t *   saver_ptr = &The_FramesSavers_Array[index];

    FlowSplicer_t * splicer_ptr = do_get_splicer_ptr(saver_ptr);

    if (strncmp(aNewValuePtr, "wait=", 5) == 0)
    {
        gboolean was_paused = (splicer_ptr->params.max_wait_ms == 0);

        if (frame_saver_params_parse_from_text(&splicer_ptr->params, params_specs) == TRUE)
        {
            sprintf(aDstValuePtr, "wait=%u", splicer_ptr->params.max_wait_ms);

            if ( (splicer_ptr->params.max_wait_ms == 0) && (! was_paused) )
            {
                saver_ptr->wait_state_ends_ns = 0;

                saver_ptr->frame_snap_wait_ns += INFINIT_NANOS;

                psz_note = "note=(PAUSED)";
            }
            else if ( (The_SysClock_Ptr != NULL) && was_paused )
            {
                GstClockTime  now_nanos = gst_clock_get_time(The_SysClock_Ptr);

                GstClockTime elapsed_ns = now_nanos - The_LaunchTime_ns;

                GstClockTime wait_nanos = (NANOS_PER_MILLISEC * splicer_ptr->params.max_wait_ms);

                GstClockTime next_nanos = NANOS_PER_MILLISEC * splicer_ptr->params.one_snap_ms;

                saver_ptr->wait_state_ends_ns = wait_nanos + now_nanos;

                saver_ptr->frame_snap_wait_ns = elapsed_ns + next_nanos;

                psz_note = "note=(RESUMED)";
            }
        }
        else
        {
            error = 1;
        }
    }
    else if (strncmp(aNewValuePtr, "path=", 5) == 0)
    {
        if (frame_saver_params_parse_from_text(&splicer_ptr->params, params_specs) == TRUE)
        {
            sprintf(aDstValuePtr, "path=%s", splicer_ptr->params.folder_path);
        }
        else
        {
            error = 2;
        }
    }
    else if (strncmp(aNewValuePtr, "snap=", 5) == 0)
    {
        if (frame_saver_params_parse_from_text(&splicer_ptr->params, params_specs) == TRUE)
        {
            sprintf(aDstValuePtr, "snap=%u,%u,%u",
                    splicer_ptr->params.one_snap_ms,
                    splicer_ptr->params.max_num_snaps_saved,
                    splicer_ptr->params.max_num_failed_snap);

            saver_ptr->num_saver_errors = 0;
            saver_ptr->num_saved_frames = 0;
            saver_ptr->num_snap_signals = 0;

            saver_ptr->num_stream_frames = 0;
            saver_ptr->num_stream_errors = 0;

            strcpy(saver_ptr->work_folder_path, splicer_ptr->params.folder_path);
        }
        else
        {
            error = 3;
        }
    }
    else if (strncmp(aNewValuePtr, "link=", 5) == 0)
    {
        if (frame_saver_params_parse_from_text(&splicer_ptr->params, params_specs) == TRUE)
        {
            sprintf(aDstValuePtr, "link=%s,%s,%s",
                    splicer_ptr->params.pipeline_name,
                    splicer_ptr->params.producer_name,
                    splicer_ptr->params.consumer_name);
        }
        else
        {
            error = 4;
        }
    }
    else if (strncmp(aNewValuePtr, "pads=", 5) == 0)
    {
        if (frame_saver_params_parse_from_text(&splicer_ptr->params, params_specs) == TRUE)
        {
            sprintf(aDstValuePtr, "pads=%s,%s,%s",
                    splicer_ptr->params.producer_out_pad_name,
                    splicer_ptr->params.consumer_inp_pad_name,
                    splicer_ptr->params.consumer_out_pad_name);
        }
        else
        {
            error = 5;
        }
    }

    GST_ERROR(PREFIX_FORMAT "Set_Params --- Error=%d --- NOW=(%s) %s \n", saver_ptr->instance_ID, error, aDstValuePtr, psz_note);

    return error;   // 0 is success
}


//=======================================================================================
// synopsis: result = frame_saver_filter_tester(argc, argv)
//
// performs a self-test of the frame-saver-filter --- returns 0 on success, else error
//=======================================================================================
int frame_saver_filter_tester( int argc, char ** argv )
{
    int result = do_initialize_static_resources();

    if ( result != 0 )
    {
        GST_ERROR("frame_saver_filter_tester --- do_initialize_static_resources() --- FAILED \n");
        return result;
    }

    gst_init(NULL, NULL);

    FramesSaver_t   *  saver_ptr = &The_FramesSavers_Array[0];

    FlowSplicer_t  * splicer_ptr = do_get_splicer_ptr(saver_ptr);

    SplicerParams_t * params_ptr = &splicer_ptr->params;

    saver_ptr->instance_ID = 1;

    frame_saver_params_initialize( params_ptr );

    if (frame_saver_params_parse_from_array(params_ptr, ++argv, --argc) != TRUE)
    {
        result = 2;
    }
    else if (frame_saver_params_write_to_file(params_ptr, stdout) != TRUE)
    {
        GST_ERROR("frame_saver_filter_tester --- report parameters used --- FAILED \n");
        result = 3;
    }
    else
    {
        result = (int) do_pipeline_create_instance(saver_ptr);  // 0 is success

        if (result != 0)
        {
            GST_ERROR("frame_saver_filter_tester --- create pipeline --- ERRORS=(%d) \n", result);
        }
        else if (do_pipeline_test_run_main_loop(saver_ptr) != TRUE)
        {
            result =4;
        }

        do_frame_saver_element_cleanup( saver_ptr );
    }

    return result;   // returns 0 on success
}
