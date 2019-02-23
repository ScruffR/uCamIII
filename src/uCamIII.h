/* *************************************************************************************

Library for 4D Systems uCam-III TTL Camera 
(also works with uCam-II - where features are supported)

http://4dsystems.com.au/product/uCAM_III/
http://www.4dsystems.com.au/productpages/uCAM-III/downloads/uCAM-III_datasheet_R_1_0.pdf

Overview:
The library implements most functions the uCamIII provides according to datasheet, 
only baudrate selection is omitted due to the fact that up to 115200 the autodetection 
works fine and higher baudrates available for the camera are usually not supported by 
the microcontrollers using this library.

It also supports hardware and software serial ports (e.g. `ParticleSoftSerial` on the 
Particle side or `SoftwareSerial` and `NewSoftSerial` for Arduino) by use of 
C++ templates.

Like this:
uCamIII<USARTSerial>        ucamHW(Serial1);
uCamIII<ParticleSoftSerial> ucamSW(new ParticleSoftSerial(D0, D1));
// or
ParticleSoftSerial          pss(D0, D1);
uCamIII<ParticleSoftSerial> ucamSW(pss);

----------------------------------------------------------------------------------------

MIT License

Copyright (c) 2018 ScruffR (Andreas Rothenwänder)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

************************************************************************************* */

#ifndef _UCAMIII_h_
#define _UCAMIII_h_

#if defined(PARTICLE)
 #include <Particle.h>
 #if (SYSTEM_VERSION < 0x00060100)
  #error "uCamIII library requires system target 0.6.1 or higher"
 #endif
 //#include <ParticleSoftSerial.h>
#else
 #include "Arduino.h"
 #include "SoftwareSerial.h"
#endif

enum uCamIII_CMD
{ uCamIII_CMD_INIT          = 0x01
, uCamIII_CMD_GET_PICTURE   = 0x04
, uCamIII_CMD_SNAPSHOT      = 0x05
, uCamIII_CMD_SET_PACKSIZE  = 0x06
, uCamIII_CMD_SET_BAUDRATE  = 0x07
, uCamIII_CMD_RESET         = 0x08
, uCamIII_CMD_DATA          = 0x0A
, uCamIII_CMD_SYNC          = 0x0D
, uCamIII_CMD_ACK           = 0x0E
, uCamIII_CMD_NAK           = 0x0F
, uCamIII_CMD_SET_FREQ      = 0x13
, uCamIII_CMD_SET_CBE       = 0x14
, uCamIII_CMD_SLEEP         = 0x15
, uCamIII_STARTBYTE         = 0xAA
, uCamIII_DONT_CARE         = 0xFF
};

enum uCamIII_IMAGE_FORMAT 
{ uCamIII_RAW_8BIT          = 0x03
, uCamIII_RAW_16BIT_RGB565  = 0x06
, uCamIII_COMP_JPEG         = 0x07
, uCamIII_RAW_16BIT_CRYCBY  = 0x08
};

enum uCamIII_RES
{ uCamIII_80x60             = 0x01
, uCamIII_160x120           = 0x03
, uCamIII_160x128           = 0x03
, uCamIII_320x240           = 0x05
, uCamIII_640x480           = 0x07
, uCamIII_128x96            = 0x08
, uCamIII_128x128           = 0x09
};

enum uCamIII_PIC_TYPE
{ uCamIII_TYPE_SNAPSHOT     = 0x01
, uCamIII_TYPE_RAW          = 0x02
, uCamIII_TYPE_JPEG         = 0x05
};

enum uCamIII_SNAP_TYPE 
{ uCamIII_SNAP_JPEG         = 0x00
, uCamIII_SNAP_RAW          = 0x01
};

enum uCamIII_RESET_TYPE
{ uCamIII_RESET_FULL        = 0x00
, uCamIII_RESET_STATE       = 0x01
, uCamIII_RESET_FORCE       = 0xFF
};

enum uCamIII_FREQ
{ uCamIII_50Hz              = 0x00
, uCamIII_60Hz              = 0x01
};

enum uCamIII_CBE
{ uCamIII_MIN               = 0x00  // Exposure -2
, uCamIII_LOW               = 0x01  //          -1
, uCamIII_DEFAULT           = 0x02  //           0
, uCamIII_HIGH              = 0x03  //          +1
, uCamIII_MAX               = 0x04  //          +2
};

enum uCamIII_ERROR 
{ uCamIII_ERROR_PIC_TYPE    = 0x01
, uCamIII_ERROR_PIC_UPSCALE = 0x02   
, uCamIII_ERROR_PIC_SCALE   = 0x03   
, uCamIII_ERROR_UNEXP_REPLY = 0x04   
, uCamIII_ERROR_PIC_TIMEOUT = 0x05   
, uCamIII_ERROR_UNEXP_CMD   = 0x06   
, uCamIII_ERROR_JPEG_TYPE   = 0x07   
, uCamIII_ERROR_JPEG_SIZE   = 0x08   
, uCamIII_ERROR_PIC_FORMAT  = 0x09   
, uCamIII_ERROR_PIC_SIZE    = 0x0A   
, uCamIII_ERROR_PARAM       = 0x0B   
, uCamIII_ERROR_SEND_TIMEOUT= 0x0C   
, uCamIII_ERROR_CMD_ID      = 0x0D   
, uCamIII_ERROR_PIC_NOT_RDY = 0x0F   
, uCamIII_ERROR_PKG_NUM     = 0x10   
, uCamIII_ERROR_PKG_SIZE    = 0x11
, uCamIII_ERROR_CMD_HEADER  = 0xF0   
, uCamIII_ERROR_CMD_LENGTH  = 0xF1   
, uCamIII_ERROR_PIC_SEND    = 0xF5   
, uCamIII_ERROR_CMD_SEND    = 0xFF   
};

typedef int (*uCamIII_callback)(uint8_t* buffer, int len, int id);

class uCamIII_Base {
public:
  uCamIII_Base(Stream& cameraStream, int resetPin = -1, uint32_t timeout = 500) 
  : _cameraStream(cameraStream), _resetPin(resetPin), _timeout(timeout), _imageSize(0), _packageSize(64), _packageNumber(0), _lastError(0) { } 

  long              sync(int maxTry = 60);
  long              getPicture(uCamIII_PIC_TYPE type = uCamIII_TYPE_JPEG);
  long              getJpegData(uint8_t *buffer, int len, uCamIII_callback callback = NULL, int package = -1);
  long              getRawData(uint8_t *buffer, int len, uCamIII_callback callback = NULL);
  void              hardReset();
  
  inline long       setImageFormat(uCamIII_IMAGE_FORMAT format = uCamIII_COMP_JPEG, uCamIII_RES resolution = uCamIII_640x480)
                    { Log.trace(__FUNCTION__); return sendCmdWithAck(uCamIII_CMD_INIT, 0x00, format, resolution, resolution); }
  inline long       takeSnapshot(uCamIII_SNAP_TYPE type = uCamIII_SNAP_JPEG, uint16_t frame = 0)
                    { Log.trace(__FUNCTION__); long r = sendCmdWithAck(uCamIII_CMD_SNAPSHOT, type, frame & 0xFF, (frame >> 8) & 0xFF); delay(_timeout); return r; }
  inline long       reset(uCamIII_RESET_TYPE type = uCamIII_RESET_FULL, bool force = true)
                    { Log.trace(__FUNCTION__); return sendCmdWithAck(uCamIII_CMD_RESET, type, 0x00, 0x00, force ? uCamIII_RESET_FORCE : 0x00); }
  inline long       setFrequency(uCamIII_FREQ frequency = uCamIII_50Hz)
                    { Log.trace(__FUNCTION__); return sendCmdWithAck(uCamIII_CMD_SET_FREQ, frequency); }
  inline long       setCBE(uCamIII_CBE contrast = uCamIII_DEFAULT, uCamIII_CBE brightness = uCamIII_DEFAULT, uCamIII_CBE exposure = uCamIII_DEFAULT)
                    { Log.trace(__FUNCTION__); return sendCmdWithAck(uCamIII_CMD_SET_CBE, contrast, brightness, exposure); }
  inline long       setIdleTime(uint8_t seconds = 15)
                    { Log.trace(__FUNCTION__); return sendCmdWithAck(uCamIII_CMD_SLEEP, seconds); }
  inline long       setPackageSize(uint16_t size = 64)
                    { Log.trace(__FUNCTION__); return sendCmdWithAck(uCamIII_CMD_SET_PACKSIZE, 0x08, size & 0xFF, (size >> 8) & 0xFF) ? (_packageSize = size) : 0; }
  inline uint8_t    getLastError() 
                    { Log.trace(__FUNCTION__); return _lastError; }
  
protected:
  Stream&           _cameraStream;
  int               _resetPin;
  unsigned long     _timeout;
  long              _imageSize;
  short             _packageSize;
  unsigned short    _packageNumber;
  uint8_t           _lastError;
  
  long              sendCmd(uCamIII_CMD cmd, uint8_t p1 = 0, uint8_t p2 = 0, uint8_t p3 = 0, uint8_t p4 = 0);
  long              sendCmdWithAck(uCamIII_CMD cmd, uint8_t p1 = 0, uint8_t p2 = 0, uint8_t p3 = 0, uint8_t p4 = 0);
  long              expectPackage(uCamIII_CMD pkg, uint8_t option = uCamIII_DONT_CARE);
  long              init();
  
#if defined(PARTICLE)
  inline void       yield() 
                    { Log.trace(__FUNCTION__); Particle.process(); }
#else
  inline void       yield() 
                    { }
#endif
};


template <class serial>
class uCamIII : public uCamIII_Base {
public:
  uCamIII(serial& camera, int resetPin = -1, uint32_t timeout = 500) 
  : uCamIII_Base(camera, resetPin, timeout), _cameraInterface(camera) { } 
  uCamIII(serial *camera, int resetPin = -1, uint32_t timeout = 500) 
  : uCamIII_Base(*camera, resetPin, timeout), _cameraInterface(*camera) { } 

  long init(int baudrate = 9600) { 
    Log.trace("uCAMIII: %s", __FUNCTION__);
    _cameraInterface.end();
    delay(100);
    _cameraInterface.begin(baudrate);
    _cameraInterface.setTimeout(_timeout);
    delay(100);
    return uCamIII_Base::init();
  }

private:
  serial& _cameraInterface;
};

#endif
