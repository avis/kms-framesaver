/*
 * =================================================================================================
 * File:        FrameSaverVideoFilterImpl.cpp
 *
 * History:     1. 2016-11-25   JBendor     Created as a class derived from kurento::FilterImpl
 *              2. 2016-12-14   JBendor     Updated
 *
 * Copyright (c) 2016 TELMATE INC. All Rights Reserved. Proprietary and confidential.
 *               Unauthorized copying of this file is strictly prohibited.
 * =================================================================================================
 */

#include <gst/gst.h>
#include "MediaPipeline.hpp"
#include "MediaPipelineImpl.hpp"
#include <FrameSaverVideoFilterImplFactory.hpp>
#include "FrameSaverVideoFilterImpl.hpp"


#define GST_CAT_DEFAULT     kurento_frame_saver_video_filter_impl
GST_DEBUG_CATEGORY_STATIC   (GST_CAT_DEFAULT);
#define GST_DEFAULT_NAME    "framesavervideofilter"


namespace kurento
{
namespace module
{
namespace framesavervideofilter
{


void FrameSaverVideoFilterImpl::postConstructor ()
{
    FilterImpl::postConstructor ();
}


FrameSaverVideoFilterImpl::FrameSaverVideoFilterImpl (const boost::property_tree::ptree &config,
                                                      std::shared_ptr<MediaPipeline> mediaPipeline)
                          : FilterImpl (config, std::dynamic_pointer_cast<MediaPipelineImpl> (mediaPipeline) )
{
    mGstreamElementPtr = NULL;

    initializeInstance(true);
}


FrameSaverVideoFilterImpl::~FrameSaverVideoFilterImpl()
{
    releaseResources(true);
}


bool FrameSaverVideoFilterImpl::startPipelinePlaying()
{
    std::unique_lock <std::recursive_mutex>  locker (mRecursiveMutex);

    bool is_ok = FALSE;     // (mGstreamElementPtr != NULL);

    if (is_ok)
    {
        GstElement * pipeline_ptr = (GstElement *) gst_element_get_parent( mGstreamElementPtr );

        if (pipeline_ptr != NULL)
        {
            GstState pipeline_state = GST_STATE_NULL;

            gst_element_get_state(pipeline_ptr, &pipeline_state, NULL, 0);

            if (pipeline_state != GST_STATE_PLAYING)
            {
                gst_element_set_state ( pipeline_ptr, GST_STATE_PLAYING );
            }
        }
        else
        {
            is_ok = false;
        }
    }

    mLastErrorDetails.assign( is_ok ? "" : "ERROR" );

    return is_ok;
}


bool FrameSaverVideoFilterImpl::stopPipelinePlaying()
{
    std::unique_lock <std::recursive_mutex>  locker (mRecursiveMutex);

    bool is_ok = FALSE; // (mGstreamElementPtr != NULL);

    if (is_ok)
    {
        GstElement * pipeline_ptr = (GstElement *) gst_element_get_parent( mGstreamElementPtr );

        if (pipeline_ptr != NULL)
        {
            GstState pipeline_state = GST_STATE_NULL;

            gst_element_get_state(pipeline_ptr, &pipeline_state, NULL, 0);

            if (pipeline_state == GST_STATE_PLAYING)
            {
                gst_element_set_state ( pipeline_ptr, GST_STATE_READY );
            }
        }
        else
        {
            is_ok = false;
        }
    }

    mLastErrorDetails.assign( is_ok ? "" : "ERROR" );

    return is_ok;
}


std::string FrameSaverVideoFilterImpl::getLastError()
{
    std::unique_lock <std::recursive_mutex>  locker(mRecursiveMutex);

    std::string  last_error( mLastErrorDetails.c_str() );

    mLastErrorDetails.assign("");

    return last_error;
}


std::string FrameSaverVideoFilterImpl::getElementsNamesList()
{
    std::unique_lock <std::recursive_mutex>  locker(mRecursiveMutex);

    std::string  names_separated_by_tabs;

    bool is_ok = (mGstreamElementPtr != NULL);

    GstElement * pipeline_ptr = is_ok ? (GstElement *) gst_element_get_parent(mGstreamElementPtr) : NULL;

    GstIterator * iterate_ptr = pipeline_ptr ? gst_bin_iterate_elements(GST_BIN(pipeline_ptr)) : NULL;

    is_ok = (iterate_ptr != NULL);

    if (is_ok)
    {
        gchar * name_ptr = gst_element_get_name(pipeline_ptr);

        names_separated_by_tabs.append(name_ptr ? name_ptr : "PipelineNameIsNull");
        names_separated_by_tabs.append("\t");

        g_free(name_ptr);

        GValue element_as_value = G_VALUE_INIT;

        while (gst_iterator_next(iterate_ptr, &element_as_value) == GST_ITERATOR_OK)
        {
            name_ptr = gst_element_get_name( g_value_get_object(&element_as_value) );

            names_separated_by_tabs.append(name_ptr ? name_ptr : "ElementNameIsNull");
            names_separated_by_tabs.append("\t");

            g_free(name_ptr);

            g_value_reset( &element_as_value );
        }

        g_value_unset( &element_as_value );

        gst_iterator_free(iterate_ptr);
    }

    mLastErrorDetails.assign( is_ok ? "" : "ERROR" );

    return names_separated_by_tabs;
}


std::string FrameSaverVideoFilterImpl::getParamsList()
{
    static const char * names[] = { "wait", "snap", "link", "pads", "path", "note", NULL };

    std::string  params_separated_by_tabs;

    int index = -1;

    while ( names[++index] != NULL )
    {
        std::string param_text = getParam( std::string(names[index]) );

        bool is_ok = mLastErrorDetails.empty();

        if (! is_ok)
        {
            params_separated_by_tabs.assign("");
            break;
        }

        params_separated_by_tabs.append( names[index] ); 
        params_separated_by_tabs.append( "=" );

        params_separated_by_tabs.append( param_text.c_str() );
        params_separated_by_tabs.append( "\t" );
    }

    return params_separated_by_tabs;
}


std::string FrameSaverVideoFilterImpl::getParam(const std::string & rParamName)
{
    std::unique_lock <std::recursive_mutex>  locker (mRecursiveMutex);

    gchar * text_ptr = NULL;

    bool is_ok = (mGstreamElementPtr != NULL);

    if (is_ok)
    {
        g_object_get( G_OBJECT(mGstreamElementPtr), rParamName.c_str(), & text_ptr, NULL );

        is_ok = (text_ptr != NULL);
    }

    std::string param_value(is_ok ? text_ptr : "");

    mLastErrorDetails.assign(is_ok ? "" : "ERROR");

    return param_value;
}


bool FrameSaverVideoFilterImpl::setParam(const std::string & rParamName, const std::string & rNewValue)
{
    std::unique_lock <std::recursive_mutex>  locker (mRecursiveMutex);

    std::string param_text = getParam(rParamName);

    bool is_ok = mLastErrorDetails.empty();

    if (is_ok)
    {
        g_object_set( G_OBJECT(mGstreamElementPtr), rParamName.c_str(), rNewValue.c_str(), NULL );
    }

    mLastErrorDetails.assign( is_ok ? "" : "ERROR" );

    return is_ok;
}


bool FrameSaverVideoFilterImpl::initializeInstance(bool isNewInstance)
{
    std::unique_lock <std::recursive_mutex>  locker (mRecursiveMutex);

    mLastErrorDetails.assign("");

    g_object_set (element, "filter-factory", GST_DEFAULT_NAME, NULL);

    g_object_get (G_OBJECT (element), "filter", &mGstreamElementPtr, NULL);

    if (mGstreamElementPtr == NULL) 
    {
        throw KurentoException (MEDIA_OBJECT_NOT_AVAILABLE, "Media Object " GST_DEFAULT_NAME " not available");
    }

    // There is no need to reference the element because its life cycle is the same as the filter's
    g_object_unref (mGstreamElementPtr);

    return true;
}


bool FrameSaverVideoFilterImpl::releaseResources(bool isDelete)
{ 
    std::unique_lock <std::recursive_mutex>  locker (mRecursiveMutex);

    return true;
}


// factory method is defined AFTER all virtual public functions have a body because the auto-generated class FrameSaverVideoFilter delares them Abstract
MediaObjectImpl * FrameSaverVideoFilterImplFactory::createObject (const boost::property_tree::ptree & config, std::shared_ptr<MediaPipeline> parent) const
{
    MediaObjectImpl * object_ptr = (MediaObjectImpl *) new FrameSaverVideoFilterImpl (config, parent);

    return object_ptr;
}


FrameSaverVideoFilterImpl::StaticConstructor FrameSaverVideoFilterImpl::Private_Static_Constructor;

FrameSaverVideoFilterImpl::StaticConstructor::StaticConstructor()
{
    GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, GST_DEFAULT_NAME, 0, GST_DEFAULT_NAME);
}

} // ends namespace: framesavervideofilter

} // ends namespace: module

} // ends namespace: kurento

