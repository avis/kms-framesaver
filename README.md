+ =======================================| 
+ File: README.md
+ =======================================| 
+ 
+ Note: The folder "Documents" has files describing how to build the Kurento Module, and how to build the Java web-apps.
+ 
+ =======================================| 
+ 
+ Example: How the "Frame-Saver" is used in the Java app ".\Validation\kms_tutorial_apps\APP-Java-For-player-with-one-way-frame-saver"
+ 
+   A1. The relevant source for this example are: "PlayerHandler.java" and "FrameSaverPluginProxy.java"
+   A2. In "PlayerHandler" line 62 defines the member "mFrameSaverProxy" --- private  FrameSaverPluginProxy mFrameSaverProxy = null;
+   A3. In "PlayerHandler" handling some events (start,stop,pause,resume) involves calling the method "mFrameSaverProxy.setOneParam()"
+   A4. In "PlayerHandler" at line 194, on the "start" event, the "mFrameSaverProxy" is instantiated
+   A5. In "PlayerHandler" at line 196, on the "start" event, mFrameSaverProxy.setParams() is called with "null" to set default params. 
+   A6. In "PlayerHandler" at line 200, mFrameSaverProxy.setOneParam() is called (conditioned on success at line 196) to set a folder-path.
+   A7. In "PlayerHandler" at line 205, mFrameSaverProxy.connectWith() is called to connect "VideoProducer-to-FrameSaver-to-VideoConsumer".
+ 
+   B1. In "FrameSaverPluginProxy" lines 028 thru 032 define the array of default parameters --- every parameter has a descriptive comment.
+   B2. In "FrameSaverPluginProxy" lines 117 thru 129 define the method "setOneParam(name,value)" --- returned boolean is true iff success.
+   B3. In "FrameSaverPluginProxy" lines 132 thru 160 define the method "setParams(paramsArray)" ---- returned boolean is true iff success.
+   B4. In "FrameSaverPluginProxy" lines 062 thru 094 define the method "connectWith(from,into)" ---- returned boolean is true iff success.
+   B5. In "FrameSaverPluginProxy" the method "connectWith(from,into)" merits the better name "connectInBetween(producer,consumer)" TODO!!!
+   B6. In "FrameSaverPluginProxy" the method "getElementNames()" is not relevant because Kurento encloses the "FrameSaver" in a container.
+   B7. In "FrameSaverPluginProxy" the method "setSpliceLinkAfter()" is not relevant for the same reasson as mentioned in B6.
+ 
+ =======================================| 
+ 
+ Parameters Used To Configure Kurento's Frame-Saver-Module --- Note: the word "auto" can be used to indciate a default value.
+ 
+   C1: Parameter "wait=T" starts an idle-wait period of T millis before resuming saving frames --- unlimited wait when T is 0.
+   C2: Parameter "snap=T,N,E" sets frame-saving interval to T millis, maximum saved frames count to N, maximum error count to E.
+   C3: Parameter "path=FOLDER" sets the path of the working folder where a sub-folder is created whenever an idle-wait period ends.
+   C4: Parameter "link=ELEM1,ELEM2,PIPE" means splicing the Gstreamer's pipeline named PIPE between elements named ELEM1 and ELEM2.
+   C5: Parameter "pads=FROM,INTO,NEXT" defines the names of the Gstreamer-Element-Pads for the placment of a Tee Splicing element.
+   C6: Note: Java apps use "link=auto,auto,auto" and "pads=auto,auto,auto" because KMS prevents splicing the Gstreamer's pipeline.
+ 
+ =======================================| 
+ 
+ About The Saved Image Files:
+ 
+   D1: The "FrameSaver" creates a new sub-folder whenever it is ready to save the first frame after a "idle-standby/idle-waiting" period.
+   D2: The name of a sub-folder has the format "IMAGES_YYYYMMDD_HHmmss" --- The paremeter "path=" defines the parent folder (deafult=CWD).
+   D3: The name of a saved image file has the format "FMT_WDTxHGTxPIX.@SSSS_MMM.#INDEX.png" --- example: "RGB_640x480x8.@0006_041.#4.png".
+   D4: The meaning of C3: Format=RGB(8,8,8), 640 pixels width, 480 pixel height, 8 bits per color, image #4, 6.041 seconds after idle end.
+   D5: One saved image file holds a PNG structure for exactly one captured video frame.
+ 
+ =======================================| 
+ 
+ ends file: README.md
