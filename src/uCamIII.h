#ifndef _4DS_UCAM_h_
#define _4DS_UCAM_h_

#if defined(PARTICLE)
 #include <Particle.h>
 #if (SYSTEM_VERSION < 0x00060100)
  #error "4DS_uCam library requires system target 0.6.1 or higher"
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
, uCamIII_RAW_16BIT_R5G6B5  = 0x06
, uCamIII_COMP_JPEG         = 0x07
, uCamIII_RAW_16BIT_CRYCbY  = 0x08
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
  : _cameraStream(cameraStream), _resetPin(resetPin), _timeout(timeout), _imageSize(0), _packageSize(64), _lastError(0), _packageNumber(0) { } 

  long              sync(int maxTry = 60);
  long              getPicture(uCamIII_PIC_TYPE type = uCamIII_TYPE_JPEG);
  long              getJpegData(uint8_t *buffer, int len, uCamIII_callback callback = NULL, int package = -1);
  long              getRawData(uint8_t *buffer, int len, uCamIII_callback callback = NULL);
  void              hardReset();
  
  inline long       setImageFormat(uCamIII_IMAGE_FORMAT format = uCamIII_COMP_JPEG, uCamIII_RES resolution = uCamIII_640x480)
                    { Log.trace(__FUNCTION__); return sendCmdWithAck(uCamIII_CMD_INIT, 0x00, format, uCamIII_COMP_JPEG ? 0x09 : resolution, uCamIII_COMP_JPEG ? resolution : 0x03); }
  inline long       takeSnapshot(uCamIII_SNAP_TYPE type = uCamIII_SNAP_JPEG, uint16_t frame = 0)
                    { Log.trace(__FUNCTION__); long r = sendCmdWithAck(uCamIII_CMD_SNAPSHOT, type, frame & 0xFF, (frame >> 8) & 0xFF); delay(100); return r; }
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
  uint32_t          _timeout;
  uint32_t          _imageSize;
  uint16_t          _packageSize;
  uint16_t          _packageNumber;
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
  : uCamIII_Base(camera, resetPin, timeout), _cameraInterface(camera) { Log.trace(__FUNCTION__); } 
  uCamIII(serial *camera, int resetPin = -1, uint32_t timeout = 500) 
  : uCamIII_Base(*camera, resetPin, timeout), _cameraInterface(*camera) { Log.trace(__FUNCTION__); } 

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
