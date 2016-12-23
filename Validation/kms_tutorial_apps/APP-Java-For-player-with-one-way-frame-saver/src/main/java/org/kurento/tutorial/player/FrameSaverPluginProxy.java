/* 
 * ======================================================================================
 * File:        FrameSaverPluginProxy.c
 * 
 * History:     1. 2016-11-28   JBendor     Created
 *              5. 2016-12-22   JBendor     Updated
 *
 * Description: Implements a proxy for a Kurento module: FrameSaverVideoFilterPlugin
 *
 * Copyright (c) 2016 TELMATE INC. All Rights Reserved. Proprietary and confidential.
 *               Unauthorized copying of this file is strictly prohibited.
 * ======================================================================================
 */

package org.kurento.tutorial.player;

import org.kurento.client.MediaElement;
import org.kurento.client.MediaPipeline;
import org.kurento.module.framesavervideofilter.FrameSaverVideoFilter;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;


public class FrameSaverPluginProxy
{
    private String TheDefaultParams[] = { "wait=1000",             // wait 1 second before saving --- forever iff 0
                                          "snap=1000,9,2",         // 1000 ms intervals, limit 9 frames or 2 errors 
                                          "link=auto,auto,auto",   // link in pipeline of the elements to be spliced
                                          "pads=auto,auto,auto",   // pads spliced by TEE (occurs after the waiting period)
                                          "path=auto"              // working folder (frames will be stored in sub-folders)
                                        };  
    
    private static final Logger     TheLogger = LoggerFactory.getLogger(FrameSaverPluginProxy.class);
    
    private MediaPipeline           mMediaPipeline = null;
    
    private FrameSaverVideoFilter   mFramesSaverFilter = null;
    
    
    public FrameSaverPluginProxy(MediaPipeline aPipeline)
    {
        TheLogger.info("FrameSaverPluginProxy.newInstance(%s) \n", (aPipeline != null)  ? "aPipeline" : "NULL" );

        try
        { 
            mFramesSaverFilter = new FrameSaverVideoFilter.Builder(aPipeline).build();

            mMediaPipeline = aPipeline;

            TheLogger.info("FramesSaver {} CREATED", (mFramesSaverFilter != null)  ? "WAS" : "NOT" );
        }
        catch (Exception ex)
        {
            TheLogger.info( "FramesSaver EXCEPTION: " + ex.getMessage() );           
        }

        return;
    }
    
    
    public boolean connectWith(MediaElement aFromElement, MediaElement aIntoElement)
    {
        boolean is_ok = true;
        
        try
        {
            if (mFramesSaverFilter != null) 
            {
                aFromElement.connect(mFramesSaverFilter);
                mFramesSaverFilter.connect(aIntoElement);
                TheLogger.info( "CONNECTED:   aFromElement-->>--mFramesSaverFilter-->>--aIntoElement" );           
                String element_names = mFramesSaverFilter.getElementsNamesList().replaceAll("\t",",");
                TheLogger.info("mFramesSaverFilter.ElementsNames: ({}) \r\n", element_names);
            }
            else
            {
                aFromElement.connect(aIntoElement);
                TheLogger.info( "CONNECTED:   aFromElement-->>--aIntoElement" );           
            }

            if (mMediaPipeline == null)   // always false --- disabled
            {
                TheLogger.info("PipelineTopology CONNECTED \r\n {} \r\n ------ \r\n", mMediaPipeline.getGstreamerDot());
            }
        }
        catch(Exception ex)
        {
            TheLogger.info( "EXCEPTION: " + ex.getMessage() );           
            is_ok = false;
        }            
        
        return is_ok; 
    }    

    
    public boolean isUsable()
    {
        return (mFramesSaverFilter != null);
    }
    

    public String getElementNames()
    {
        String element_names = "?";
        
        
        if (mFramesSaverFilter != null) 
        {
            element_names = mFramesSaverFilter.getElementsNamesList();
        }       

        return element_names;
    }
    
    
    public boolean setOneParam(String aName, String aValue)
    {
        boolean is_ok = false;
        
        if (mFramesSaverFilter != null) 
        {
            is_ok = mFramesSaverFilter.setParam(aName, aValue);

            TheLogger.info("FrameSaverPluginProxy.setOneParam: name={}  value={}  ok={}", aName, aValue, is_ok);
        }       

        return is_ok;
    }

        
    public boolean setParams(String aParamsArray[])
    {
        boolean is_ok = isUsable();
        
        if (! is_ok)
        {
            return false;               
        }
        
        
        if (aParamsArray == null)
        {
            aParamsArray = TheDefaultParams;
        }
        
        for (int index=0;  is_ok && (index < aParamsArray.length);  ++index)
        {
            String parts[] = aParamsArray[index].split("=");
            
            is_ok = (parts.length == 2);
            
            if (is_ok)
            { 
            	is_ok = setOneParam(parts[0], parts[1]);
            }
        }
        
        return is_ok;
    }

    
    public boolean setSpliceLinkAfter(MediaElement producerElement)
    {
        String producer_name = "auto";
        String consumer_name = "auto";
        
        boolean is_ok = isUsable();

        if (! is_ok)
        {
            TheLogger.info("ERROR! isUsable()");
            
            return false;
        }
        
        String element_names = getElementNames();
        
        if ( (element_names == null) || element_names.isEmpty() )
        {
            TheLogger.info("ERROR! mFramesSaverFilter.getElementsNamesList()");
            
            return false;            
        }

        TheLogger.info("ElementsNameList: ({}) \r\n", element_names);
        
        if (is_ok)
        {
            String link_specs = String.format("auto, %s, %s", producer_name, consumer_name);
            
            is_ok = setOneParam("link", link_specs);
        }        
        
        return is_ok;        // TODO
    }
    
}
