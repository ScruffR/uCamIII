# Library for 4D Systems uCam-III TTL Camera 
(also works with uCam-II - where features are supported)

 - http://4dsystems.com.au/product/uCAM_III/
 - http://www.4dsystems.com.au/productpages/uCAM-III/downloads/uCAM-III_datasheet_R_1_0.pdf

## Overview:
The library implements most functions the uCamIII provides according to datasheet, 
only baudrate selection is omitted due to the fact that up to 115200 the autodetection 
works fine and higher baudrates available for the camera are usually not supported by 
the microcontrollers using this library.

It also supports hardware and software serial ports (e.g. `ParticleSoftSerial` on the 
Particle side or `SoftwareSerial` and `NewSoftSerial` for Arduino) by use of 
C++ templates.

Like this:
```
uCamIII<USARTSerial>        ucamHW(Serial1);
uCamIII<ParticleSoftSerial> ucamSW(new ParticleSoftSerial(D0, D1));
// or
ParticleSoftSerial          pss(D0, D1);
uCamIII<ParticleSoftSerial> ucamSW(pss);
```

## Example Firmware uCamTest:
## Example Firmware uCamTest:
This sketch demonstrates how to use the uCamIII library.
It will provide a `Particle.function("snap")` that can be triggered with parameters
`GRAY8` (default for wrong parameters too), `RGB16`, `UYVY16` and `JPG` to take a pic and 
send it via `Serial` or `TCP` (provided via `Particle.function("setTarget")`) where it 
can be dumped into a file.
For the TCP data sink you need to be running a server like the provided ['imageReceiver.js'](/server/imageReceiver.js)
(run the server from its file location via `node ./imageReceiver.js`) and inform the 
device of the IP and port for the server. This is done via `Particle.function("setServer")`
in the form `###.###.###.###:port`.

For WiFi devices it also provides a Webserver which lets you select image format and
resolution and displays the image. 
For non-JPG formats a BMP header is prepended to allow use in <img> tag.

The server address can be retrieved via Particle.variale("IP")

![WebView](/img/uCamTest.png)

