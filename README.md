# Library for 4D Systems uCam-III TTL Camera (also works with uCam-II - where features are supported)

http://4dsystems.com.au/product/uCAM_III/

### Overview

The library implements most functions the uCamIII provides according to [datasheet](http://www.4dsystems.com.au/productpages/uCAM-III/downloads/uCAM-III_datasheet_R_1_0.pdf), only baudrate selection is omitted due to the fact that up to 115200 the autodetection works fine and higher baudrates available for the camera are usually not supported by the microcontrollers using this library.

It also supports hardware and software serial ports (e.g. `ParticleSoftSerial` on the Particle side or `SoftwareSerial` and `NewSoftSerial` for Arduino) by use of C++ templates.

Like this
```
uCamIII<ParticleSoftSerial> ucam(new ParticleSoftSerial(D0, D1));
```
