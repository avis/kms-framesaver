/*
 * =================================================================================================
 * File:        FrameSaverVideoFilterImpl.hpp
 *
 * History:     1. 2016-11-25   JBendor     Created a a class derived from kurento::FilterImpl
 *              2. 2016-12-14   JBendor     Updated
 *
 * Copyright (c) 2016 TELMATE INC. All Rights Reserved. Proprietary and confidential.
 *               Unauthorized copying of this file is strictly prohibited.
 * =================================================================================================
 */

#ifndef __FRAME_SAVER_VIDEO_FILTER_IMPL_HPP__
#define __FRAME_SAVER_VIDEO_FILTER_IMPL_HPP__

#include "FilterType.hpp"
#include "FilterImpl.hpp"
#include "FrameSaverVideoFilter.hpp"

#include <boost/property_tree/ptree.hpp>
#include <jsonrpc/JsonSerializer.hpp>
#include <KurentoException.hpp>
#include <EventHandler.hpp>
#include <mutex>


namespace kurento
{
    namespace module
    {
        namespace framesavervideofilter
        {
            class FrameSaverVideoFilterImpl;
        }
    }
}


namespace kurento
{
    void Serialize (std::shared_ptr<kurento::module::framesavervideofilter::FrameSaverVideoFilterImpl> & object, JsonSerializer & serializer);
}


namespace kurento
{
    class MediaPipelineImpl;
}


namespace kurento
{

namespace module
{

namespace framesavervideofilter
{

class FrameSaverVideoFilterImpl : public FilterImpl, public virtual FrameSaverVideoFilter
{

public:

    FrameSaverVideoFilterImpl (const boost::property_tree::ptree & ref_config, std::shared_ptr<MediaPipeline> ptr_Pipeline);

    virtual ~FrameSaverVideoFilterImpl();                           // virtual d'tor

    virtual bool startPipelinePlaying();                                    // starts pipeline PLAYING

    virtual bool stopPipelinePlaying();                                     // changes PLAYING to READY

    virtual std::string getLastError();                                     // returns non-empty for errors

    virtual std::string getElementsNamesList();                             // returns NamesSeparatedByTabs

    virtual std::string getParamsList();                                    // returns ParamsSeparatedByTabs

    virtual std::string getParam(const std::string & rParamName);           // returns empty if invalid name

    virtual bool setParam(const std::string & rParamName, const std::string & rNewValue); // FALSE if failed

    // The bodies of next three methods are automatically implemented by the code generator
    virtual void Serialize (JsonSerializer &serializer);
    virtual bool connect (const std::string &eventType,  std::shared_ptr<EventHandler> handler);
    virtual void invoke (std::shared_ptr<MediaObjectImpl> obj, const std::string &methodName, const Json::Value &params, Json::Value &response);

protected:
    virtual void postConstructor ();

    virtual bool initializeInstance(bool isNewInstance);

    virtual bool releaseResources(bool isDelete);

private:
    std::recursive_mutex    mRecursiveMutex;
    std::string             mLastErrorDetails;
    GstElement            * mGstreamElementPtr;

    class StaticConstructor
    {
        public:  StaticConstructor();
    };

    static StaticConstructor  Private_Static_Constructor;

};


} // ends namespace: framesavervideofilter

} // ends namespace: module

} // ends namespace: kurento

#endif //  __FRAME_SAVER_VIDEO_FILTER_IMPL_HPP__

