#include <uCamIII.h>

// http://www.4dsystems.com.au/productpages/uCAM-III/downloads/uCAM-III_datasheet_R_1_0.pdf
// CONNAMD FORMAT                        sync  cmd   par1  par2  par3  par4     			// par1 		par2		par3 		par4
//const char uCamIII_INIT[]           = {0xAA, 0x01, 0x00, 0x07, 0x09, 0x07};				// 0x00			img format	raw res		JPEG res				
//const char uCamIII_GET_PICTURE[]    = {0xAA, 0x04, 0x01, 0x00, 0x00, 0x00};				// pic type		0x00		0x00		0x00
//const char uCamIII_SNAPSHOT[]       = {0xAA, 0x05, 0x00, 0x00, 0x00, 0x00};				// snap type	skip loByte	hiByte		0x00
//const char uCamIII_SET_PACKSIZE[]   = {0xAA, 0x06, 0x08, 0x06, 0x00, 0x00};	            // 0x08			size loByte	hiByte		0x00
//const char uCamIII_SET_BAUDRATE[]   = {0xAA, 0x07, 0x00, 0x00, 0x00, 0x00};				// 1.divider	2.divider	0x00		0x00
//const char uCamIII_RESET[]          = {0xAA, 0x08, 0x00, 0x00, 0x00, 0x00};				// reset type	0x00		0x00		0xFF (force) 0x## (normal)
//const char uCamIII_DATA[]           = {0xAA, 0x0A, 0x00, 0x00, 0x00, 0x00};				// data type	len byte0	byte1		byte2
//const char uCamIII_SYNC[]           = {0xAA, 0x0D, 0x00, 0x00, 0x00, 0x00};				// 0x00			0x00		0x00		0x00
//const char uCamIII_ACK[]            = {0xAA, 0x0E, 0x00, 0x00, 0x00, 0x00};				// cmd2ack		ack count	pkID loByte	hiByte
//const char uCamIII_NAK[]            = {0xAA, 0x0F, 0x00, 0x00, 0x00, 0x00};				// 0x00			nak count	error no	0x00
//const char uCamIII_SET_FREQ[]       = {0xAA, 0x13, 0x00, 0x00, 0x00, 0x00};				// freq type	0x00		0x00		0x00
//const char uCamIII_SET_CBE[]        = {0xAA, 0x14, 0x00, 0x00, 0x00, 0x00};				// contrast		brightness	exposure	0x00
//const char uCamIII_SLEEP[]          = {0xAA, 0x15, 0x00, 0x00, 0x00, 0x00};				// timeout		0x00		0x00		0x00

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
    if (count = sendCmdWithAck(uCamIII_CMD_SYNC)) break;
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
  && _imageSize == _cameraStream.readBytes((char*)buffer, _imageSize)
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
  
  Log.warn("timeout: %02X %02X %02X %02X %02X %02X (%d)", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], _timeout);
  return 0;
}
