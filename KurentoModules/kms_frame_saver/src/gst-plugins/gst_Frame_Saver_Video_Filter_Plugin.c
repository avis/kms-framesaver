/*
 * ======================================================================================
 * File:        gst_Frame_Saver_Video_Filter_Plugin.c
 *
 * Purpose:     Kurento+Gstreamer plugin-filter --- uses "Frame-Saver" for full behavior.
 *
 * History:     1. 2016-11-25   JBendor     Created as copy of "gst_Frame_Saver_Plugin.c"
 *              2. 2016-11-25   JBendor     Adapted to _IS_KURENTO_FILTER_ being defined
 *              3. 2016-12-22   JBendor     Updated
 *
 * Copyright (c) 2016 TELMATE INC. All Rights Reserved. Proprietary and confidential.
 *               Unauthorized copying of this file is strictly prohibited.
 * ======================================================================================
 */

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include "gst_Frame_Saver_Video_Filter_Plugin.h"

#include <string.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <glib/gstdio.h>


GST_DEBUG_CATEGORY_STATIC       (gst_frame_saver_plugin_debug_category);
#define GST_CAT_DEFAULT         gst_frame_saver_plugin_debug_category


#define  _IS__SAVING_FRAMES_


typedef enum
{
    e_DBG_RARE = 1,    // rarely
    e_DBG_FREQ = 2,    // frequently
    e_DBG_MUST = 3     // always

} DebugLevel_e;

static DebugLevel_e  The_Debug_Level_Cutoff = e_DBG_MUST;


typedef enum
{
    e_PROP_0,

    e_PROP_WAIT,    // "wait=MillisWaitBeforeNextFrameSnap"
    e_PROP_SNAP,    // "snap=MillisIntervals,MaxNumSnaps,MaxNumFails"
    e_PROP_LINK,    // "link=PipelineName,ProducerName,ConsumerName"
    e_PROP_PADS,    // "pads=ProducerOut,ConsumerInput,ConsumerOut"
    e_PROP_PATH,    // "path=PathForWorkingFolderForSavedImageFiles"
    e_PROP_NOTE,    // "note=none or note=MostRecentError"
    e_PROP_SILENT   // silent=0 or silent=1 --- 1 disables messages

} PLUGIN_PARAMS_e;


enum
{
    e_FIRST_SIGNAL = 0,
    e_FINAL_SIGNAL

} PLUGIN_SIGNALS_e;


typedef struct _GstFrameSaverPluginPrivate
{
    gboolean     is_silent;
    guint        num_buffs;
    guint        num_drops;
    guint        num_notes;
    gchar        sz_wait[30],
                 sz_snap[30],
                 sz_link[100],
                 sz_pads[100],
                 sz_path[300],
                 sz_note[300],
                 sz_caps[300];

} GstFrameSaverPluginPrivate;

#define VIDEO_SRC_CAPS      GST_VIDEO_CAPS_MAKE("{ BGR }")
#define VIDEO_SINK_CAPS     GST_VIDEO_CAPS_MAKE("{ BGR }")

extern GType gst_frame_saver_plugin_get_type(void);     // body defined by macro: G_DEFINE_TYPE
static void  gst_frame_saver_plugin_init (GstFrameSaverPlugin * aPtr);  // initialize instance

G_DEFINE_TYPE_WITH_CODE (GstFrameSaverPlugin,                                               \
                         gst_frame_saver_plugin,                                            \
                         GST_TYPE_VIDEO_FILTER,                                             \
                         GST_DEBUG_CATEGORY_INIT (gst_frame_saver_plugin_debug_category,    \
                                                  THIS_PLUGIN_NAME, 0,                      \
                                                  "debug category for FrameSaverPlugin element"));

#define GET_PRIVATE_STRUCT_PTR(obj)    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                          \
                                                                    GST_TYPE_OF_FRAME_SAVER_PLUGIN, \
                                                                    GstFrameSaverPluginPrivate))


#ifdef _IS__SAVING_FRAMES_

    extern int Frame_Saver_Filter_Detach(GstElement * pluginPtr);
    extern int Frame_Saver_Filter_Attach(GstElement * pluginPtr);
    extern int Frame_Saver_Filter_Receive_Buffer(GstElement * pluginPtr, GstBuffer * aBufferPtr, const char * aCapsTextPtr);
    extern int Frame_Saver_Filter_Transition(GstElement * pluginPtr, GstStateChange aTransition) ;
    extern int Frame_Saver_Filter_Set_Params(GstElement * pluginPtr, const gchar * aNewValuePtr, gchar * aPrvSpecsPtr);

#else

    static int Frame_Saver_Filter_Attach(GstElement * pluginPtr)
    {
        GST_LOG("%s --- %s \n", THIS_PLUGIN_NAME, __func__);
        return 0;
    }
    static int Frame_Saver_Filter_Detach(GstElement * pluginPtr)
    {
        GST_LOG("%s --- %s \n", THIS_PLUGIN_NAME, __func__);
        return 0;
    }
    static int Frame_Saver_Filter_Receive_Buffer(GstElement * pluginPtr, GstBuffer * aBufferPtr, const char * aCapsTextPtr)
    {
        GST_LOG("%s --- %s \n", THIS_PLUGIN_NAME, __func__);
        return 0;
    }
    static int Frame_Saver_Filter_Transition(GstElement * pluginPtr, GstStateChange aTransition)
    {
        GST_LOG("%s --- %s \n", THIS_PLUGIN_NAME, __func__);
        return 0;
    }
    static int Frame_Saver_Filter_Set_Params(GstElement * pluginPtr, const gchar * aNewValuePtr, gchar * aPrvSpecsPtr)
    {
        GST_LOG("%s --- %s \n", THIS_PLUGIN_NAME, __func__);
        return 0;
    }

#endif


static void initialize_instance(GstFrameSaverPlugin * aPluginPtr, GstFrameSaverPluginPrivate * aPrivatePtr);


static GstClock      * The_Sys_Clock_Ptr = NULL;
static GstClockTime    The_Startup_Nanos = 0;


static guint64 Get_Runtime_Nanosec()
{
    if (The_Sys_Clock_Ptr == NULL)
    {
        The_Sys_Clock_Ptr = gst_system_clock_obtain();

        The_Startup_Nanos = gst_clock_get_time (The_Sys_Clock_Ptr);

        GST_LOG("\n%s\n", THIS_PLUGIN_NAME);
    }

    GstClockTime  now_nanos = gst_clock_get_time( The_Sys_Clock_Ptr );

    GstClockTime elapsed_ns = now_nanos -The_Startup_Nanos;

    return (guint64) elapsed_ns;
}


static guint Get_Runtime_Millisec()
{
    #define  NANOS_PER_MILLISEC  ((guint64) (1000L * 1000L))

    return (guint) (Get_Runtime_Nanosec() / NANOS_PER_MILLISEC);
}


static guint DBG1_Print(DebugLevel_e severityLevel, const gchar * aTextPtr, gint aValue)
{
    if ( severityLevel >= The_Debug_Level_Cutoff )
    {
        guint elapsed_ms = Get_Runtime_Millisec();

        if (aTextPtr != NULL)
        {
            GST_LOG("@%d --- %s --- %s --- (%d)", elapsed_ms, THIS_PLUGIN_NAME, aTextPtr, aValue);
        }

        GST_LOG("\n");
    }

    return 0;
}


static void gst_frame_saver_plugin_init(GstFrameSaverPlugin * aPluginPtr)
{
    The_Sys_Clock_Ptr = NULL;

    DBG1_Print( e_DBG_MUST, __func__, 0 );

    initialize_instance(aPluginPtr, GET_PRIVATE_STRUCT_PTR(aPluginPtr));

    Frame_Saver_Filter_Attach( GST_ELEMENT(aPluginPtr) );

    return;
}


static void gst_frame_saver_plugin_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
    gchar * psz_now = NULL;

    GstFrameSaverPlugin        *  ptr_filter = GST_FRAME_SAVER_PLUGIN(object);

    GstFrameSaverPluginPrivate * ptr_private = GET_PRIVATE_STRUCT_PTR(ptr_filter);

    GST_OBJECT_LOCK(ptr_filter);

    switch (prop_id)
    {
    case e_PROP_SILENT:
        ptr_private->is_silent = g_value_get_boolean(value);
        DBG1_Print( e_DBG_MUST, ptr_private->is_silent ? "Silent=TRUE" : "Silent=FALSE", 0 );
        break;

    case e_PROP_WAIT:
        snprintf( ptr_private->sz_wait, sizeof(ptr_private->sz_wait), "wait=%s", g_value_get_string(value) );
        psz_now = ptr_private->sz_wait;
        break;

    case e_PROP_SNAP:
        snprintf( ptr_private->sz_snap, sizeof(ptr_private->sz_snap), "snap=%s", g_value_get_string(value) );
        psz_now = ptr_private->sz_snap;
        break;

    case e_PROP_LINK:
        snprintf( ptr_private->sz_link, sizeof(ptr_private->sz_link), "link=%s", g_value_get_string(value) );
        psz_now = ptr_private->sz_link;
        break;

    case e_PROP_PADS:
        snprintf( ptr_private->sz_pads, sizeof(ptr_private->sz_pads), "pads=%s", g_value_get_string(value) );
        psz_now = ptr_private->sz_pads;
        break;

    case e_PROP_PATH:
        snprintf( ptr_private->sz_path, sizeof(ptr_private->sz_path), "path=%s", g_value_get_string(value) );
        psz_now = ptr_private->sz_path;
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }

    if (psz_now != NULL)
    {
        gint result = Frame_Saver_Filter_Set_Params( GST_ELEMENT(ptr_filter), psz_now, psz_now );

        DBG1_Print( e_DBG_RARE, psz_now, result);

        ptr_private->num_buffs = 0;
        ptr_private->num_drops = 0;
        ptr_private->num_notes = 0;
    }

    GST_OBJECT_UNLOCK(ptr_filter);

    DBG1_Print( e_DBG_RARE, __func__, prop_id );

    return;
}


static void gst_frame_saver_plugin_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
    GstFrameSaverPlugin        *  ptr_filter = GST_FRAME_SAVER_PLUGIN(object);

    GstFrameSaverPluginPrivate * ptr_private = GET_PRIVATE_STRUCT_PTR(ptr_filter);

    GST_OBJECT_LOCK(ptr_filter);

    switch (prop_id)
    {
        case e_PROP_SILENT:
            g_value_set_boolean(value, ptr_private->is_silent);
            break;

        case e_PROP_WAIT:
            g_value_set_string(value, ptr_private->sz_wait);
            break;

        case e_PROP_SNAP:
            g_value_set_string(value, ptr_private->sz_snap);
            break;

        case e_PROP_LINK:
            g_value_set_string(value, ptr_private->sz_link);
            break;

        case e_PROP_PADS:
            g_value_set_string(value, ptr_private->sz_pads);
            break;

        case e_PROP_PATH:
            g_value_set_string(value, ptr_private->sz_path);
            break;

        case e_PROP_NOTE:
            g_value_set_string(value, ptr_private->sz_note);
            strcpy(ptr_private->sz_note, "note=none");
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }

    GST_OBJECT_UNLOCK(ptr_filter);

    DBG1_Print( e_DBG_RARE, __func__, prop_id );

    return;
}


static void gst_frame_saver_plugin_finalize (GObject * object)
{
    Frame_Saver_Filter_Detach( GST_ELEMENT(object) );

    G_OBJECT_CLASS (gst_frame_saver_plugin_parent_class)->finalize(object);

    GST_DEBUG_OBJECT (GST_FRAME_SAVER_PLUGIN(object), "finalize");

    DBG1_Print( e_DBG_MUST, __func__, 0 );
    DBG1_Print( e_DBG_MUST, NULL, 0 );

    return;
}


static void gst_frame_saver_plugin_dispose (GObject * object)
{
    Frame_Saver_Filter_Detach( GST_ELEMENT(object) );

    G_OBJECT_CLASS (gst_frame_saver_plugin_parent_class)->dispose(object);

    GST_DEBUG_OBJECT (GST_FRAME_SAVER_PLUGIN(object), "dispose");

    DBG1_Print( e_DBG_MUST, __func__, 0 );
    DBG1_Print( e_DBG_MUST, NULL, 0 );

    return;
}


static gboolean KMS_frame_saver_plugin_start (GstBaseTransform * aTransPtr)
{
    GstElement * ptr_element = (GstElement *) GST_ELEMENT(aTransPtr);

    if ( Frame_Saver_Filter_Attach(ptr_element) == 0 )
    {
        GstFrameSaverPlugin        * ptr_filter = GST_FRAME_SAVER_PLUGIN(aTransPtr);

        GstFrameSaverPluginPrivate * ptr_private = GET_PRIVATE_STRUCT_PTR(ptr_filter);

        Frame_Saver_Filter_Set_Params( ptr_element, ptr_private->sz_wait, ptr_private->sz_wait );
        Frame_Saver_Filter_Set_Params( ptr_element, ptr_private->sz_snap, ptr_private->sz_snap );
        Frame_Saver_Filter_Set_Params( ptr_element, ptr_private->sz_link, ptr_private->sz_link );
        Frame_Saver_Filter_Set_Params( ptr_element, ptr_private->sz_pads, ptr_private->sz_pads );
        Frame_Saver_Filter_Set_Params( ptr_element, ptr_private->sz_path, ptr_private->sz_path );

        Frame_Saver_Filter_Transition( ptr_element, GST_STATE_CHANGE_NULL_TO_READY);
    }

    GST_DEBUG_OBJECT (GST_FRAME_SAVER_PLUGIN(aTransPtr), "start");

    DBG1_Print( e_DBG_MUST, __func__, 0 );
    DBG1_Print( e_DBG_MUST, NULL, 0 );

    return TRUE;    // must return TRUE to receive frames
}


static gboolean KMS_frame_saver_plugin_stop (GstBaseTransform * aTransPtr)
{
    Frame_Saver_Filter_Detach( GST_ELEMENT(aTransPtr) );

    GST_DEBUG_OBJECT (GST_FRAME_SAVER_PLUGIN(aTransPtr), "stop");

    DBG1_Print( e_DBG_MUST, __func__, 0 );
    DBG1_Print( e_DBG_MUST, NULL, 0 );

    return TRUE;
}


static gboolean KMS_frame_saver_plugin_set_info ( GstVideoFilter  * aFilterPtr,
                                                  GstCaps         * in_caps_ptr,
                                                  GstVideoInfo    * in_info_ptr,
                                                  GstCaps         * out_caps_ptr,
                                                  GstVideoInfo    * out_info_ptr )
{
    GstFrameSaverPlugin        *  ptr_filter = GST_FRAME_SAVER_PLUGIN(aFilterPtr);

    GstFrameSaverPluginPrivate * ptr_private = GET_PRIVATE_STRUCT_PTR(ptr_filter);

    gchar                      * psz_in_caps = in_caps_ptr ? gst_caps_to_string(in_caps_ptr) : NULL;

    snprintf( ptr_private->sz_caps, sizeof(ptr_private->sz_caps), "%s", (psz_in_caps ? psz_in_caps : "") );

    g_free(psz_in_caps);

    Frame_Saver_Filter_Transition( GST_ELEMENT(aFilterPtr), GST_STATE_CHANGE_NULL_TO_READY );

    GST_DEBUG_OBJECT (GST_FRAME_SAVER_PLUGIN(aFilterPtr), "set_info");

    DBG1_Print( e_DBG_MUST, __func__, 0 );
    DBG1_Print( e_DBG_MUST, NULL, 0 );

    return TRUE;
}


static GstFlowReturn KMS_frame_saver_plugin_transform_frame_ip(GstVideoFilter * aFilterPtr, GstVideoFrame * aFramePtr)
{
    GstFrameSaverPlugin        *  ptr_filter = GST_FRAME_SAVER_PLUGIN(aFilterPtr);

    GstFrameSaverPluginPrivate * ptr_private = GET_PRIVATE_STRUCT_PTR(ptr_filter);

    if ( ptr_private->sz_caps[0] == 0 )
    {
        GstCaps * caps_ptr = gst_video_info_to_caps ( &aFramePtr->info );

        gchar   * psz_caps = caps_ptr ? gst_caps_to_string(caps_ptr) : NULL;

        g_free(caps_ptr);

        snprintf( ptr_private->sz_caps, sizeof(ptr_private->sz_caps), "%s", (psz_caps ? psz_caps : "") );

        g_free(psz_caps);
    }

    ptr_private->num_buffs += 1;

    DBG1_Print( e_DBG_RARE, __func__, ptr_private->num_buffs);

    Frame_Saver_Filter_Receive_Buffer( GST_ELEMENT(aFilterPtr), aFramePtr->buffer, ptr_private->sz_caps );

    return GST_FLOW_OK;
}


static void gst_frame_saver_plugin_class_init(GstFrameSaverPluginClass * klass)
{
    GParamFlags param_flags = (GParamFlags) ((G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    GObjectClass            *        gobject_class_ptr = G_OBJECT_CLASS(klass);
    GstVideoFilterClass     *   video_filter_class_ptr = GST_VIDEO_FILTER_CLASS(klass);
    GstBaseTransformClass   * base_transform_class_ptr = GST_BASE_TRANSFORM_CLASS(klass);

    gobject_class_ptr->set_property = gst_frame_saver_plugin_set_property;
    gobject_class_ptr->get_property = gst_frame_saver_plugin_get_property;
    gobject_class_ptr->finalize     = gst_frame_saver_plugin_finalize;
    gobject_class_ptr->dispose      = gst_frame_saver_plugin_dispose;

    base_transform_class_ptr->start = GST_DEBUG_FUNCPTR (KMS_frame_saver_plugin_start);
    base_transform_class_ptr->stop  = GST_DEBUG_FUNCPTR (KMS_frame_saver_plugin_stop);

    video_filter_class_ptr->set_info = GST_DEBUG_FUNCPTR (KMS_frame_saver_plugin_set_info);
    video_filter_class_ptr->transform_frame_ip = GST_DEBUG_FUNCPTR (KMS_frame_saver_plugin_transform_frame_ip);

    gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
                                        gst_pad_template_new ("src",
                                                              GST_PAD_SRC,
                                                              GST_PAD_ALWAYS,
                                                              gst_caps_from_string (VIDEO_SRC_CAPS)));

    gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
                                        gst_pad_template_new ("sink",
                                                              GST_PAD_SINK,
                                                              GST_PAD_ALWAYS,
                                                              gst_caps_from_string (VIDEO_SINK_CAPS)));

    g_object_class_install_property(gobject_class_ptr,
                                    e_PROP_WAIT,
                                    g_param_spec_string("wait",
                                                        "wait=MillisWaitBeforeNextFrameSnap",
                                                        "wait before snapping another frame",
                                                        "3000",
                                                        param_flags));

    g_object_class_install_property(gobject_class_ptr,
                                    e_PROP_SNAP,
                                    g_param_spec_string("snap",
                                                        "snap=millisecInterval,maxNumSnaps,maxNumFails",
                                                        "snap and save frames as RGB images in PNG files",
                                                        "1000,0,0",
                                                        param_flags));

    g_object_class_install_property(gobject_class_ptr,
                                    e_PROP_LINK,
                                    g_param_spec_string("link",
                                                        "link=pipelineName,producerName,consumerName",
                                                        "insert TEE between producer and consumer elements",
                                                        "auto,auto,auto",
                                                        param_flags));

    g_object_class_install_property(gobject_class_ptr,
                                    e_PROP_PADS,
                                    g_param_spec_string("pads",
                                                        "pads=producerOut,consumerInput,consumerOut",
                                                        "pads used by the producer and consumer elements",
                                                        "src,sink,src",
                                                        param_flags));

    g_object_class_install_property(gobject_class_ptr,
                                    e_PROP_PATH,
                                    g_param_spec_string("path",
                                                        "path=path-of-working-folder-for-saved-images",
                                                        "path of working folder for saved image files",
                                                        "auto",
                                                        param_flags));

    g_object_class_install_property(gobject_class_ptr,
                                    e_PROP_NOTE,
                                    g_param_spec_string("note",
                                                        "note=MostRecentErrorCodition",
                                                        "most recent error",
                                                        "none",
                                                        G_PARAM_READABLE));

    g_object_class_install_property(gobject_class_ptr,
                                    e_PROP_SILENT,
                                    g_param_spec_boolean("silent",
                                                         "Silent or Verbose",
                                                         "Silent is 1/True --- Verbose is 0/False",
                                                         FALSE,
                                                         G_PARAM_READWRITE));

    gst_element_class_set_details_simple(GST_ELEMENT_CLASS(klass),
                                         THIS_PLUGIN_NAME,                  // name to launch
                                         "Frame-Saver-Video-Filter",        // classification
                                         "Saves Frames (Can Insert TEE)",   // description
                                         "Author <<a.TODO@hostname.org>>"); // author info

    #ifdef _IS_KURENTO_FILTER_

        g_type_class_add_private (klass, sizeof (GstFrameSaverPluginPrivate));

    #else   // regular Gstreamer plugin --- create static pads --- set callbacks

        gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass), gst_static_pad_template_get(&The_Src_Pad_Template));

        gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass), gst_static_pad_template_get(&The_Sink_Pad_Template));

        GST_ELEMENT_CLASS(gst_frame_saver_plugin_parent_class)->change_state = gst_frame_saver_plugin_change_state;

        GST_ELEMENT_CLASS(klass)->change_state = gst_frame_saver_plugin_change_state;

    #endif  // _IS_KURENTO_FILTER_


    The_Sys_Clock_Ptr = NULL;

    DBG1_Print( e_DBG_RARE, __func__, 0 );

    return;
}


/*
 * entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean register_this_plugin(GstPlugin * aPluginPtr)
{
    The_Sys_Clock_Ptr = NULL;

    DBG1_Print( e_DBG_RARE, __func__, 0 );

    return gst_element_register (aPluginPtr, THIS_PLUGIN_NAME, GST_RANK_NONE, GST_TYPE_OF_FRAME_SAVER_PLUGIN);
}


// gstreamer looks for this structure to register plugins
GST_PLUGIN_DEFINE ( GST_VERSION_MAJOR,
                    GST_VERSION_MINOR,
                    FrameSaverPlugin,
                    "saves-image-frames",
                    register_this_plugin,
                    PLUGIN_VERSION, "LGPL", "GStreamer", "http://gstreamer.net/")


static void initialize_instance(GstFrameSaverPlugin * aPluginPtr, GstFrameSaverPluginPrivate * aPrivatePtr)
{
    strcpy(aPrivatePtr->sz_wait, "wait=2000");
    strcpy(aPrivatePtr->sz_snap, "snap=0,0,0");
    strcpy(aPrivatePtr->sz_link, "link=Live,auto,auto");
    strcpy(aPrivatePtr->sz_pads, "pads=auto,auto,auto");
    strcpy(aPrivatePtr->sz_path, "path=auto");
    strcpy(aPrivatePtr->sz_note, "note=none");
    strcpy(aPrivatePtr->sz_caps, "");

    aPrivatePtr->num_buffs = 0;
    aPrivatePtr->num_drops = 0;
    aPrivatePtr->num_notes = 0;
    aPrivatePtr->is_silent = TRUE;

    #ifdef _IS_KURENTO_FILTER_

        aPluginPtr->priv = aPrivatePtr;

    #else   // regular Gstreamer plugin --- create pads --- set callbacks --- set video properties

        ptr_private->srcpad  = gst_pad_new_from_static_template( &The_Src_Pad_Template, "src" );
        ptr_private->sinkpad = gst_pad_new_from_static_template( &The_Sink_Pad_Template, "sink" );

        gst_pad_set_event_function(ptr_private->sinkpad, GST_DEBUG_FUNCPTR(gst_frame_saver_plugin_sink_event));
        gst_pad_set_chain_function(ptr_private->sinkpad, GST_DEBUG_FUNCPTR(gst_frame_saver_plugin_chain));
        gst_pad_set_query_function(ptr_private->sinkpad, GST_DEBUG_FUNCPTR(gst_frame_saver_plugin_src_query));

        GST_PAD_SET_PROXY_CAPS(ptr_private->sinkpad);
        GST_PAD_SET_PROXY_CAPS(ptr_private->srcpad);

    #endif

    if (aPluginPtr == NULL)    // always FALSE, suppress warnings on unused statics
    {
        KMS_frame_saver_plugin_transform_frame_ip(NULL, NULL);

        #ifndef _IS_KURENTO_FILTER_
            gst_frame_saver_plugin_src_query(NULL, NULL, NULL);
            gst_frame_saver_plugin_chain(NULL, NULL, NULL);
            gst_frame_saver_plugin_sink_event(NULL, NULL, NULL);
            gst_frame_saver_plugin_change_state(NULL, GST_STATE_CHANGE_READY_TO_NULL);
        #endif
    }

    return;
}


#ifndef _IS_KURENTO_FILTER_

static gboolean gst_frame_saver_plugin_sink_event(GstPad * pad, GstObject * parent, GstEvent * aEventPtr)
{
    gboolean is_ok;

    GstCaps * ptr_caps;

    DBG1_Print( e_DBG_RARE, __func__, (gint) GST_EVENT_TYPE(aEventPtr) );

    switch (GST_EVENT_TYPE(aEventPtr))
    {
    case GST_EVENT_CAPS: // do something with the caps --- forward
        gst_event_parse_caps(aEventPtr, & ptr_caps);
        is_ok = gst_pad_event_default(pad, parent, aEventPtr);
        break;

    case GST_EVENT_EOS: // end-of-stream, close down all stream leftovers here
        is_ok = gst_pad_event_default(pad, parent, aEventPtr);
        break;

    default:
        is_ok = gst_pad_event_default(pad, parent, aEventPtr);
        break;
    }

    return is_ok;
}


// Gstreamer chain function --- this function does the actual processing
static GstFlowReturn gst_frame_saver_plugin_chain(GstPad * pad, GstObject * parent, GstBuffer * buf)
{
    GstFrameSaverPlugin        *  ptr_filter = GST_FRAME_SAVER_PLUGIN(parent);

    GstFrameSaverPluginPrivate * ptr_private = GET_PRIVATE_STRUCT_PTR(ptr_filter);

    #ifdef _IS_KURENTO_FILTER_
        GstFlowReturn  result = GST_FLOW_OK;
    #else
        GstFlowReturn  result = gst_pad_push(ptr_private->srcpad, buf);
    #endif

    ptr_private->num_drops += (result == GST_FLOW_OK) ? 0 : 1;
    ptr_private->num_buffs += 1;

    DBG1_Print( e_DBG_RARE, __func__, (gint) ptr_private->num_buffs );

    if (ptr_private->is_silent == FALSE)
    {
        GST_LOG("%s --- Push --- (%u/%u) \n", THIS_PLUGIN_NAME, ptr_private->num_buffs, ptr_private->num_drops);
    }

    if (ptr_private->sz_snap[5] > '0')  // is_frame_saver)
    {
        if ( ptr_private->sz_caps[0] == 0 )
        {
            GstCaps * caps_ptr = gst_pad_get_current_caps (pad);

            gchar   * psz_caps = caps_ptr ? gst_caps_to_string(caps_ptr) : NULL;

            snpritf(ptr_private->sz_caps, sizeof(ptr_private->sz_caps), "%s", psz_caps ? psz_caps : "");

            gst_object_unref(caps_ptr);

            g_free(psz_caps);
        }

        Frame_Saver_Filter_Receive_Buffer(GST_ELEMENT(ptr_filter), buf, ptr_private->sz_caps);
    }

    return result;  // anythin except GST_FLOW_OK could halt flow in the pipeline
}


static gboolean gst_frame_saver_plugin_src_query(GstPad *pad, GstObject *parent, GstQuery *query)
{
    gboolean ret;

    DBG1_Print( e_DBG_RARE, __func__, (gint) GST_QUERY_TYPE(query) );

    switch (GST_QUERY_TYPE(query))
    {
    case GST_QUERY_CAPS: /* report the supported caps here */
    default:
        ret = gst_pad_query_default(pad, parent, query);
        break;
    }
    return ret;
}


static GstStateChangeReturn gst_frame_saver_plugin_change_state(GstElement *element, GstStateChange transition)
{
    GstStateChangeReturn             ret_val = GST_STATE_CHANGE_SUCCESS;

    GstFrameSaverPlugin         * ptr_filter = GST_FRAME_SAVER_PLUGIN(element);

    GstFrameSaverPluginPrivate * ptr_private = GET_PRIVATE_STRUCT_PTR(ptr_filter);

    gboolean                  is_frame_saver = (ptr_private->sz_snap[5] > '0');

    DBG1_Print( e_DBG_RARE, __func__, (guint) transition );

    if (is_frame_saver)
    {
        if (transition == GST_STATE_CHANGE_NULL_TO_READY)
        {
            if ( Frame_Saver_Filter_Attach(element) == 0 )
            {
                Frame_Saver_Filter_Set_Params(element, ptr_private->sz_wait, ptr_private->sz_wait);
                Frame_Saver_Filter_Set_Params(element, ptr_private->sz_snap, ptr_private->sz_snap);
                Frame_Saver_Filter_Set_Params(element, ptr_private->sz_link, ptr_private->sz_link);
                Frame_Saver_Filter_Set_Params(element, ptr_private->sz_pads, ptr_private->sz_pads);
                Frame_Saver_Filter_Set_Params(element, ptr_private->sz_path, ptr_private->sz_path);
            }
        }

        Frame_Saver_Filter_Transition(element, transition);
    }

    if (GST_ELEMENT_CLASS(gst_frame_saver_plugin_parent_class)->change_state != NULL)
    {
        ret_val = GST_ELEMENT_CLASS(gst_frame_saver_plugin_parent_class)->change_state(element, transition);
    }

    if (ret_val != GST_STATE_CHANGE_FAILURE)
    {
        if (transition == GST_STATE_CHANGE_READY_TO_NULL)
        {
            Frame_Saver_Filter_Detach(element);
        }
    }

    return ret_val;
}

#endif // _IS_KURENTO_FILTER_

// ends file:  "gst_frame_saver_plugin_video_filter.c"

