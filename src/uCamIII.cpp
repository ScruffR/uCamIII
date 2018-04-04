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

#include <uCamIII.h>

long uCamIII_Base::init() 
{
  Log.trace("uCAMIII_Base: %s", __FUNCTION__); 
  hardReset();
  return sync();
}

long uCamIII_Base::sync(int maxTry)
{
  Log.trace(__FUNCTION__); 

  int      tries;
  int      count;
  uint32_t ms = _timeout;
  
  for (tries = 1; tries < maxTry; tries++)
  {
    _timeout = 5 + tries;                                       // start with 5ms timeout between syncs and increase by 1ms
    if ((count = sendCmdWithAck(uCamIII_CMD_SYNC))) break;
  }
  _timeout = ms;
  if (tries < maxTry)
  {
    Log.info("sync after %d tries", tries);    
    if (expectPackage(uCamIII_CMD_SYNC)) {
      sendCmd(uCamIII_CMD_ACK, uCamIII_CMD_SYNC);
      return tries;
    }
  }
  
  Log.warn("no sync");
  return 0;
}

long uCamIII_Base::getPicture(uCamIII_PIC_TYPE type)
{
  Log.trace(__FUNCTION__); 

  if (sendCmdWithAck(uCamIII_CMD_GET_PICTURE, type))
  {
    _packageNumber = 0;    
    return (_imageSize = expectPackage(uCamIII_CMD_DATA, type) & 0x00FFFFFF);   // return image size
  }
  return 0;
}


long uCamIII_Base::getJpegData(uint8_t *buffer, int len, uCamIII_callback callback, int package)
{
  Log.trace(__FUNCTION__); 

  uint16_t        id    = 0;
  uint16_t        size  = 0;
  uint32_t        ms    = millis();

  if (package >= 0) _packageNumber = package;           // request specific package
  
  if (sendCmd(uCamIII_CMD_ACK, 0x00, 0x00, _packageNumber & 0xFF, _packageNumber >> 8))
  {
    char info[4];
    char chk[2];
    if (_cameraStream.readBytes(info, sizeof(info)) == sizeof(info))
    {
      id   = info[0] | info[1] << 8;
      size = info[2] | info[3] << 8;
    }
    
    if (!size || size > len 
    || _cameraStream.readBytes((char*)buffer, size) != size
    || _cameraStream.readBytes(chk, sizeof(chk)) != sizeof(chk)
    ) 
    {                                                   // if the expected data didn't arrive in time                                                   
      delay(100);                                       // allow for extra bytes to trickle in and then
      ms = millis();                                    // flush the RX buffer and
      while(_cameraStream.read() >= 0 && millis() - ms < _timeout) yield();
      id = 0xF0F0;                                      // prepare for termination of request
    }
  }

  if (id * (_packageSize - 6) >= _imageSize || millis() - ms >= _timeout)
    sendCmd(uCamIII_CMD_ACK, 0x00, 0x00, 0xF0, 0xF0);   // report end of final data request to camera
  else
    _packageNumber = id;                                // prepare to request next package
  
  if (id < 0xF0F0)
  {
    if (callback) callback(buffer, size, id);
    return size;    
  }
  
  return 0;
}

long uCamIII_Base::getRawData(uint8_t *buffer, int len, uCamIII_callback callback)
{
  if (len >= _imageSize
  && _imageSize == (long)_cameraStream.readBytes((char*)buffer, _imageSize)
  )
  {                                                     // success -> report end of data request to camera
    sendCmd(uCamIII_CMD_ACK, uCamIII_CMD_DATA, 0x00, 0x01, 0x00);    
    if (callback) callback(buffer, _imageSize, 0);
    return _imageSize;       
  }
  else                                                  // if the expected data didn't arrive in time
  { 
    uint32_t ms;                                                                                         
    delay(100);                                         // allow for extra bytes to trickle in and then
    ms = millis();                                      // flush the RX buffer
    while(_cameraStream.read() >= 0 && millis() - ms < _timeout) yield();
  }
  
  return 0;
}

void uCamIII_Base::hardReset()
{
  Log.trace(__FUNCTION__); 

  if (_resetPin > 0)
  {
    pinMode(_resetPin, OUTPUT);
    digitalWrite(_resetPin, LOW);
    delay(10);
    pinMode(_resetPin, INPUT);
    delay(10);
  }
}

// ----------------------------------- protected ----------------------------------------

long uCamIII_Base::sendCmd(uCamIII_CMD cmd, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4)
{
  Log.trace(__FUNCTION__); 

  uint8_t buf[6] = { uCamIII_STARTBYTE, cmd, p1, p2, p3, p4 };
  Log.info("sendCmd: %02X %02X %02X %02X %02X %02X", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
  return _cameraStream.write(buf, 6);
}

long uCamIII_Base::sendCmdWithAck(uCamIII_CMD cmd, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4) 
{
  Log.trace(__FUNCTION__); 

  sendCmd(cmd, p1, p2, p3, p4);
  return expectPackage(uCamIII_CMD_ACK, cmd);
}

long uCamIII_Base::expectPackage(uCamIII_CMD pkg, uint8_t option)
{
  Log.trace("%s(%02x,%02x)", __FUNCTION__, pkg, option); 
  uint8_t buf[6];
  memset(buf, 0x00, sizeof(buf));

  if (_cameraStream.readBytes((char*)buf, sizeof(buf)) == sizeof(buf))
  {
    _lastError = 0;
    Log.trace("received: %02X %02X %02X %02X %02X %02X", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
    if (buf[1] == pkg && (buf[2] == option || option == uCamIII_DONT_CARE)) 
      return buf[3] | buf[4] << 8 | buf[5] << 16 | 0x1000000;
    else if (buf[1] == uCamIII_CMD_NAK)
      _lastError = buf[4];
  }
  
  Log.warn("timeout: %02X %02X %02X %02X %02X %02X (%lu)", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], _timeout);
  return 0;
}
