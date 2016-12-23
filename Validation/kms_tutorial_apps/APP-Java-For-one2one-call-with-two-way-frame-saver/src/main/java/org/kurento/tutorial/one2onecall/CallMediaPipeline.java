/*
 * (C) Copyright 2014 Kurento (http://kurento.org/)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

package org.kurento.tutorial.one2onecall;

import org.kurento.client.KurentoClient;
import org.kurento.client.MediaPipeline;
import org.kurento.client.WebRtcEndpoint;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Media Pipeline (WebRTC endpoints, i.e. Kurento Media Elements) and connections for the 1 to 1
 * video communication.
 * 
 * @author Boni Garcia (bgarcia@gsyc.es)
 * @author Micael Gallego (micael.gallego@gmail.com)
 * @since 4.3.1
 */
public class CallMediaPipeline {

  private MediaPipeline pipeline;
  private WebRtcEndpoint callerWebRtcEp;
  private WebRtcEndpoint calleeWebRtcEp;

  private static final Logger TheLogger = LoggerFactory.getLogger(CallMediaPipeline.class);  

  private FrameSaverPluginProxy mFrameSaverProxy1_ToCalleEE = null;
  private FrameSaverPluginProxy mFrameSaverProxy2_ToCalleRR = null;

  public CallMediaPipeline(KurentoClient kurento) {
    try {
      this.pipeline = kurento.createMediaPipeline();
      this.callerWebRtcEp = new WebRtcEndpoint.Builder(pipeline).build();
      this.calleeWebRtcEp = new WebRtcEndpoint.Builder(pipeline).build();
      
      connect_Caller_To_Callee_pass_thru_1st_Frame_Saver();
      
      connect_Callee_To_Caller_pass_thru_2nd_Frame_Saver();
      
    } catch (Throwable t) {
      if (this.pipeline != null) {
        pipeline.release();
      }
    }
  }
  
  private boolean connect_Caller_To_Callee_pass_thru_1st_Frame_Saver() 
  {      
      mFrameSaverProxy1_ToCalleEE = new FrameSaverPluginProxy(pipeline);
      
      boolean is_ok = (callerWebRtcEp != null) && (calleeWebRtcEp != null) && (mFrameSaverProxy1_ToCalleEE != null);

      TheLogger.info("CallMediaPipeline.c'tor --- (%s) \n", (is_ok  ? "OK" : "ERR") );

      boolean is_frame_saver_1_valid = mFrameSaverProxy1_ToCalleEE.setParams(null);

      if (is_frame_saver_1_valid)
      {
          is_frame_saver_1_valid = mFrameSaverProxy1_ToCalleEE.setOneParam("path", "/tmp/FrameSaver1_Caller_To_Callee");
      }

      if (is_frame_saver_1_valid)
      {
          is_frame_saver_1_valid = mFrameSaverProxy1_ToCalleEE.connectWith( callerWebRtcEp, calleeWebRtcEp );
      }

      if (is_frame_saver_1_valid)
      {

          mFrameSaverProxy1_ToCalleEE.setOneParam("wait", "1000");
      }
      else
      {
          this.callerWebRtcEp.connect(this.calleeWebRtcEp);
      }
      
      return is_frame_saver_1_valid;
  }

  private boolean connect_Callee_To_Caller_pass_thru_2nd_Frame_Saver() 
  {      
      mFrameSaverProxy2_ToCalleRR = new FrameSaverPluginProxy(pipeline);
      
      boolean is_ok = (callerWebRtcEp != null) && (calleeWebRtcEp != null) && (mFrameSaverProxy2_ToCalleRR != null);

      TheLogger.info("CallMediaPipeline.c'tor --- (%s) \n", (is_ok  ? "OK" : "ERR") );

      boolean is_frame_saver_2_valid = mFrameSaverProxy2_ToCalleRR.setParams(null);

      if (is_frame_saver_2_valid)
      {
          is_frame_saver_2_valid = mFrameSaverProxy2_ToCalleRR.setOneParam("path", "/tmp/FrameSaver2_Callee_To_Caller");
      }

      if (is_frame_saver_2_valid)
      {
          is_frame_saver_2_valid = mFrameSaverProxy2_ToCalleRR.connectWith( calleeWebRtcEp, callerWebRtcEp );
      }

      if (is_frame_saver_2_valid)
      {
          mFrameSaverProxy2_ToCalleRR.setOneParam("wait", "1000");
      }
      else
      {
          this.calleeWebRtcEp.connect(this.callerWebRtcEp);
      }
      
      return is_frame_saver_2_valid;
  }
  
  public String generateSdpAnswerForCaller(String sdpOffer) {
    return callerWebRtcEp.processOffer(sdpOffer);
  }

  public String generateSdpAnswerForCallee(String sdpOffer) {
    return calleeWebRtcEp.processOffer(sdpOffer);
  }

  public void release() {
    if (pipeline != null) {
      pipeline.release();
    }
  }

  public WebRtcEndpoint getCallerWebRtcEp() {
    return callerWebRtcEp;
  }

  public WebRtcEndpoint getCalleeWebRtcEp() {
    return calleeWebRtcEp;
  }

}
