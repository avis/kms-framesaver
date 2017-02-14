;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-; 
; File: README.md
;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-; 
; 
; Note: The folder "Documents" has files describing how to build the Kurento Module, and how to build the Java web-apps.
; 
;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-; 
; 
; Example: How the "Frame-Saver" is used in the Java app ".\Validation\kms_tutorial_apps\APP-Java-For-player-with-one-way-frame-saver"
; 
;   A1. The relevant source for this example are: "PlayerHandler.java" and "FrameSaverPluginProxy.java"
;   A2. In "PlayerHandler" line 62 defines the member "mFrameSaverProxy" --- private  FrameSaverPluginProxy mFrameSaverProxy = null;
;   A3. In "PlayerHandler" handling some events (start,stop,pause,resume) involves calling the method "mFrameSaverProxy.setOneParam()"
;   A4. In "PlayerHandler" at line 194, on the "start" event, the "mFrameSaverProxy" is instantiated
;   A5. In "PlayerHandler" at line 196, on the "start" event, mFrameSaverProxy.setParams() is called with "null" to set default params. 
;   A6. In "PlayerHandler" at line 200, mFrameSaverProxy.setOneParam() is called (conditioned on success at line 196) to set a folder-path.
;   A7. In "PlayerHandler" at line 205, mFrameSaverProxy.connectWith() is called to connect "VideoProducer-to-FrameSaver-to-VideoConsumer".
; 
;   B1. In "FrameSaverPluginProxy" lines 028 thru 032 define the array of default parameters --- every parameter has a descriptive comment.
;   B2. In "FrameSaverPluginProxy" lines 117 thru 129 define the method "setOneParam(name,value)" --- returned boolean is true iff success.
;   B3. In "FrameSaverPluginProxy" lines 132 thru 160 define the method "setParams(paramsArray)" ---- returned boolean is true iff success.
;   B4. In "FrameSaverPluginProxy" lines 062 thru 094 define the method "connectWith(from,into)" ---- returned boolean is true iff success.
;   B5. In "FrameSaverPluginProxy" the method "connectWith(from,into)" merits the better name "connectInBetween(producer,consumer)" TODO!!!
;   B6. In "FrameSaverPluginProxy" the method "getElementNames()" is not relevant because Kurento encloses the "FrameSaver" in a container.
;   B7. In "FrameSaverPluginProxy" the method "setSpliceLinkAfter()" is not relevant for the same reasson as mentioned in B6.
; 
;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-; 
; 
; About The Saved Image Files:
; 
;   C1: The "FrameSaver" creates a new sub-folder whenever it is ready to save the first frame after a "standby/waiting" period.
;   C2: The name of a sub-folder has the format "IMAGES_YYYYMMDD_HHmmss" --- The paremeter "path=" defines the parent folder (deafult=CWD).
;   C3: The name of a saved image file has the format "FMT_WDTxHGTxPIX.@SSSS_MMM.; INDEX.png" --- example: "RGB_640x480x8.@0006_041.; 4.png".
;   C4: The meaning of C3: Image format RGB(8,8,8), 640 pixels width, 480 pixel height, 8 bits per color, 4th capture at 6041 ms after 1st.
;   C5: One saved image file holds a PNG structure for exactly one captured video frame.
; 
;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-;-; 
; 
; ends file: README.md
