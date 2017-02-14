/*
 * (C) Copyright 2015 Kurento (http://kurento.org/)
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

package org.kurento.tutorial.player;

import java.io.IOException;
import java.util.concurrent.ConcurrentHashMap;

import org.kurento.client.EndOfStreamEvent;
import org.kurento.client.ErrorEvent;
import org.kurento.client.EventListener;
import org.kurento.client.IceCandidate;
import org.kurento.client.IceCandidateFoundEvent;
import org.kurento.client.KurentoClient;
import org.kurento.client.MediaPipeline;
import org.kurento.client.MediaState;
import org.kurento.client.MediaStateChangedEvent;
import org.kurento.client.PlayerEndpoint;
import org.kurento.client.VideoInfo;
import org.kurento.client.WebRtcEndpoint;
import org.kurento.commons.exception.KurentoException;
import org.kurento.jsonrpc.JsonUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.socket.CloseStatus;
import org.springframework.web.socket.TextMessage;
import org.springframework.web.socket.WebSocketSession;
import org.springframework.web.socket.handler.TextWebSocketHandler;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;

/**
 * Protocol handler for video player through WebRTC.
 *
 * @author Boni Garcia (bgarcia@gsyc.es)
 * @author David Fernandez (dfernandezlop@gmail.com)
 * @author Ivan Gracia (igracia@kurento.org)
 * @since 6.1.1
 */
public class PlayerHandler extends TextWebSocketHandler {

  @Autowired
  private KurentoClient kurento;
  
  private  FrameSaverPluginProxy mFrameSaverProxy = null;

  private final Logger log = LoggerFactory.getLogger(PlayerHandler.class);
  private final Gson gson = new GsonBuilder().create();
  private final ConcurrentHashMap<String, UserSession> users = new ConcurrentHashMap<>();

  @Override
  public void handleTextMessage(WebSocketSession session, TextMessage message) throws Exception {
    JsonObject jsonMessage = gson.fromJson(message.getPayload(), JsonObject.class);
    String sessionId = session.getId();
    log.info("Incoming Message {} --- From Session-ID {}", jsonMessage, sessionId);

    try {
      switch (jsonMessage.get("id").getAsString()) {
        case "start":
          start(session, jsonMessage);
          mFrameSaverProxy.setOneParam("snap", "2000,4,2");
          break;
        case "stop":
          mFrameSaverProxy.setOneParam("wait", "0");
          stop(sessionId);
          break;
        case "pause":
          mFrameSaverProxy.setOneParam("wait", "0");
          pause(sessionId);
          break;
        case "resume":
          mFrameSaverProxy.setOneParam("snap", "1000,8,2");
          mFrameSaverProxy.setOneParam("wait", "5");
          resume(session);
          break;
        case "doSeek":
          doSeek(session, jsonMessage);
          break;
        case "getPosition":
          getPosition(session);
          break;
        case "onIceCandidate":
          onIceCandidate(sessionId, jsonMessage);
          break;
        default:
          sendError(session, "Invalid message with id " + jsonMessage.get("id").getAsString());
          break;
      }
    } catch (Throwable t) {
      log.error("Exception handling message {} in sessionId {}", jsonMessage, sessionId, t);
      sendError(session, t.getMessage());
    }
  }

  private void start(final WebSocketSession session, JsonObject jsonMessage) {
      String videourl = jsonMessage.get("videourl").getAsString();
      log.info("Video-URL is {} --- Session-ID is {}", videourl, session.getId());

      // create the Media pipeline --- no video source or sink yet, but initial state is PLAYING
      MediaPipeline pipeline = kurento.createMediaPipeline();

      // add to pipeline two Media Elements (WebRtcEndpoint, PlayerEndpoint)
      WebRtcEndpoint webRtcEndpoint = new WebRtcEndpoint.Builder(pipeline).build();
      final PlayerEndpoint playerEndpoint = new PlayerEndpoint.Builder(pipeline, videourl).build();

      // Create and Update User Session
      final UserSession user = new UserSession();
      user.setMediaPipeline(pipeline);
      user.setWebRtcEndpoint(webRtcEndpoint);
      user.setPlayerEndpoint(playerEndpoint);
      users.put(session.getId(), user);

      // 2. WebRtcEndpoint
      // ICE candidates
      webRtcEndpoint.addIceCandidateFoundListener(new EventListener<IceCandidateFoundEvent>() {

        @Override
        public void onEvent(IceCandidateFoundEvent event) {
          JsonObject response = new JsonObject();
          response.addProperty("id", "iceCandidate");
          response.add("candidate", JsonUtils.toJsonObject(event.getCandidate()));
          try {
            synchronized (session) {
              session.sendMessage(new TextMessage(response.toString()));
            }
          } catch (IOException e) {
            log.debug(e.getMessage());
          }
        }
      });

      String sdpOffer = jsonMessage.get("sdpOffer").getAsString();
      String sdpAnswer = webRtcEndpoint.processOffer(sdpOffer);

      JsonObject response = new JsonObject();
      response.addProperty("id", "startResponse");
      response.addProperty("sdpAnswer", sdpAnswer);
      sendMessage(session, response.toString());

      webRtcEndpoint.addMediaStateChangedListener(new EventListener<MediaStateChangedEvent>() {
        @Override
        public void onEvent(MediaStateChangedEvent event) {

          if (event.getNewState() == MediaState.CONNECTED) {
            VideoInfo videoInfo = playerEndpoint.getVideoInfo();

            JsonObject response = new JsonObject();
            response.addProperty("id", "videoInfo");
            response.addProperty("isSeekable", videoInfo.getIsSeekable());
            response.addProperty("initSeekable", videoInfo.getSeekableInit());
            response.addProperty("endSeekable", videoInfo.getSeekableEnd());
            response.addProperty("videoDuration", videoInfo.getDuration());
            sendMessage(session, response.toString());
          }
        }
      });

      webRtcEndpoint.gatherCandidates();

      // 3. PlayEndpoint
      playerEndpoint.addErrorListener(new EventListener<ErrorEvent>() {
        @Override
        public void onEvent(ErrorEvent event) {
          log.info("ErrorEvent: {}", event.getDescription());
          sendPlayEnd(session);
        }
      });

      playerEndpoint.addEndOfStreamListener(new EventListener<EndOfStreamEvent>() {
        @Override
        public void onEvent(EndOfStreamEvent event) {
          log.info("EndOfStreamEvent: {}", event.getTimestamp());
          sendPlayEnd(session);
        }
      });
      
      mFrameSaverProxy = new FrameSaverPluginProxy(pipeline);
      
      boolean is_frame_saver_valid = mFrameSaverProxy.setParams(null);

      if (is_frame_saver_valid)
      {
          is_frame_saver_valid = mFrameSaverProxy.setOneParam("path", "/tmp/FrameSaver_Player_To_Viewer");
      }

      if (is_frame_saver_valid)
      {
          is_frame_saver_valid = mFrameSaverProxy.connectWith(playerEndpoint, webRtcEndpoint);
      }
      
      if (! is_frame_saver_valid)
      {
          playerEndpoint.connect(webRtcEndpoint);         
      }
      
      playerEndpoint.play();
  }
  
  private void pause(String sessionId) {
    UserSession user = users.get(sessionId);

    if (user != null) {
      user.getPlayerEndpoint().pause();
    }
  }

  private void resume(final WebSocketSession session) {
    UserSession user = users.get(session.getId());

    if (user != null) {
      user.getPlayerEndpoint().play();
      VideoInfo videoInfo = user.getPlayerEndpoint().getVideoInfo();

      JsonObject response = new JsonObject();
      response.addProperty("id", "videoInfo");
      response.addProperty("isSeekable", videoInfo.getIsSeekable());
      response.addProperty("initSeekable", videoInfo.getSeekableInit());
      response.addProperty("endSeekable", videoInfo.getSeekableEnd());
      response.addProperty("videoDuration", videoInfo.getDuration());
      sendMessage(session, response.toString());
    }
  }

  private void stop(String sessionId) {
      UserSession user = users.remove(sessionId);

    if (user != null) {
      user.release();
    }
  }

  private void doSeek(final WebSocketSession session, JsonObject jsonMessage) {
    UserSession user = users.get(session.getId());

    if (user != null) {
      try {
        user.getPlayerEndpoint().setPosition(jsonMessage.get("position").getAsLong());
      } catch (KurentoException e) {
        log.debug("The seek cannot be performed");
        JsonObject response = new JsonObject();
        response.addProperty("id", "seek");
        response.addProperty("message", "Seek failed");
        sendMessage(session, response.toString());
      }
    }
  }

  private void getPosition(final WebSocketSession session) {
    UserSession user = users.get(session.getId());

    if (user != null) {
      long position = user.getPlayerEndpoint().getPosition();

      JsonObject response = new JsonObject();
      response.addProperty("id", "position");
      response.addProperty("position", position);
      sendMessage(session, response.toString());
    }
  }

  private void onIceCandidate(String sessionId, JsonObject jsonMessage) {
    UserSession user = users.get(sessionId);

    if (user != null) {
      JsonObject jsonCandidate = jsonMessage.get("candidate").getAsJsonObject();
      IceCandidate candidate =
          new IceCandidate(jsonCandidate.get("candidate").getAsString(), jsonCandidate
              .get("sdpMid").getAsString(), jsonCandidate.get("sdpMLineIndex").getAsInt());
      user.getWebRtcEndpoint().addIceCandidate(candidate);
    }
  }

  public void sendPlayEnd(WebSocketSession session) {
    if (users.containsKey(session.getId())) {
      JsonObject response = new JsonObject();
      response.addProperty("id", "playEnd");
      sendMessage(session, response.toString());
    }
  }

  private void sendError(WebSocketSession session, String message) {
    if (users.containsKey(session.getId())) {
      JsonObject response = new JsonObject();
      response.addProperty("id", "error");
      response.addProperty("message", message);
      sendMessage(session, response.toString());
    }
  }

  private synchronized void sendMessage(WebSocketSession session, String message) {
    try {
      session.sendMessage(new TextMessage(message));
    } catch (IOException e) {
      log.error("Exception sending message", e);
    }
  }

  @Override
  public void afterConnectionClosed(WebSocketSession session, CloseStatus status) throws Exception {
    stop(session.getId());
  }
}