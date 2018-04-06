/* *************************************************************************************

# Library for 4D Systems uCam-III TTL Camera 
(also works with uCam-II - where features are supported)

http://4dsystems.com.au/product/uCAM_III/
http://www.4dsystems.com.au/productpages/uCAM-III/downloads/uCAM-III_datasheet_R_1_0.pdf

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
This sketch demonstrates how to use the uCamIII library.
It will provide a `Particle.function("snap")` that can be triggered with parameters
`GRAY8` (default for wrong parameters too), `RGB16`, `UYVY16` and `JPG` to take a pic and 
send it via `Serial` where it can be dumped into a file.

For WiFi devices it also provides a Webserver which lets you select image format and
resolution and displays the image. 
For non-JPG formats a BMP header is prepended to allow use in <img> tag.

The server address can be retrieved via Particle.variale("IP")

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

#include "uCamIII.h"
#include "WebServer.h"

//SerialLogHandler traceLog(LOG_LEVEL_TRACE);

#if Wiring_WiFi
#define COLOR_TYPE "CT"
#define RAW_RES    "RR"
#define JPG_RES    "JR"
#define CONTRAST   "cbeC"
#define BRIGHTNESS "cbeB"
#define EXPOSURE   "cbeE"

P(Page_start) = "<html><head><meta charset='utf-8'/><title>uCamIII Demo</title></head><body>\n";
P(camBody) = 
"<form id='settings' onclick='click'>"
"<table style='width: 100%;'>"
"<tr>"
"<td>"
"<label>Color Type</label>"
"<select id='" COLOR_TYPE "' name='" COLOR_TYPE "'>"
"<option value='07'>JPEG</option>"
"<option value='03'>RawGrascale8bit</option>"
"<option value='06'>RawColor16bitRGB565</option>"
"<option value='08'>RawColor16bitCrYCbY</option>"
"</select>"
"</td>"
"<td id='raw_res'>"
"<label>Raw Resolution</label>"
"<select id='" RAW_RES "' name='" RAW_RES "'>"
"<option value='03'>160x120</option>"
"<option value='09'>128x128</option>"
"<option value='08'>128x96</option>"
"<option value='01'>80x60</option>"
"</select>"
"</td>"
"<td id='jpeg_res' visible='false'>"
"<label>JPEG Resolution</label>"
"<select id='" JPG_RES "' name='" JPG_RES "'>"
"<option value='07'>640x480</option>"
"<option value='05'>320x240</option>"
"<option value='03'>160x120</option>"
"</select>"
"</td>"
"<td>"
"<input id='click' type='submit' value='Click!' rowspan='2' style='width:100% height:100%' />"
"</td>"
"</tr>"
"<tr>"
"<td>"
"<label>Contrast</label>"
"<select id='" CONTRAST "' name='" CONTRAST "'>"
"<option value='00'>Min</option>"
"<option value='01'>Low</option>"
"<option value='02' selected='true'>Default</option>"
"<option value='03'>High</option>"
"<option value='04'>Max</option>"
"</select>"
"</td>"
"<td id='raw_res'>"
"<label>Brightness</label>"
"<select id='" BRIGHTNESS "' name='" BRIGHTNESS "'>"
"<option value='00'>Min</option>"
"<option value='01'>Low</option>"
"<option value='02' selected='true'>Default</option>"
"<option value='03'>High</option>"
"<option value='04'>Max</option>"
"</select>"
"</td>"
"<td id='jpeg_res' visible='false'>"
"<label>Exposure</label>"
"<select id='" EXPOSURE "' name='" EXPOSURE "'>"
"<option value='00'>-2</option>"
"<option value='01'>-1</option>"
"<option value='02' selected='true'>0</option>"
"<option value='03'>+1</option>"
"<option value='04'>+2</option>"
"</select>"
"</td>"
"</tr>"
"</table>"
"</form>"
"<br/>"
"<img src='image.dat' />"
"<br/>"
"<a href='image.dat'>download image</a>"
"<br/>"
"<h3><font color='red'>16bit raw images will be displayed with distorted colors</font></h3>"
"<h4>Chrome doesn't cope well with raw images at all, better use other browser</h4>"
"<br/>To view with correct colors use an image viewer that supports"
"<br/>RGB565 or CrYCbY/UYVY encoding (e.g. GIMP)"
"<br/>"
"<a href='http://www.4dsystems.com.au/productpages/uCAM-III/downloads/uCAM-III_datasheet_R_1_0.pdf'>uCamIII datasheet</a>"
"<br/><br/>"
;
P(Page_end) = "</body></html>";

P(script) = 
"<script>" 
"document.getElementById('" COLOR_TYPE "').value='%02d';"
"document.getElementById('" RAW_RES "').value='%02d';"
"document.getElementById('" JPG_RES "').value='%02d';"
"document.getElementById('" CONTRAST "').value='%02d';"
"document.getElementById('" BRIGHTNESS "').value='%02d';"
"document.getElementById('" EXPOSURE "').value='%02d';"
"</script>"
;


/* This creates an instance of the webserver.  By specifying a prefix
 * of "", all pages will be at the root of the server. */
const char PREFIX[] = "";
const int  NAMELEN  = 32;
const int  VALUELEN = 32;

WebServer webserver(PREFIX, 80);

void defaultCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete);
void imageCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete);
void sendBmpHeader(WebServer &server, int width, int height, int bits, int encoding = 0, int padding = 0);  
#endif

long takePicture(uCamIII_SNAP_TYPE snap    = uCamIII_SNAP_RAW, 
                 uCamIII_IMAGE_FORMAT fmt  = uCamIII_RAW_8BIT, 
                 uCamIII_RES res           = uCamIII_80x60, 
                 uCamIII_CBE contrast      = uCamIII_DEFAULT,
                 uCamIII_CBE brightness    = uCamIII_DEFAULT,
                 uCamIII_CBE exposure      = uCamIII_DEFAULT,
                 uCamIII_callback callback = NULL);

uCamIII<USARTSerial> ucam(Serial1, A0);                             // use HW Serial1 and A0 as reset pin for uCamIII
// or
// uCamIII<ParticleSoftSerial> ucam(new ParticleSoftSerial(D0, D1));
// or
// ParticleSoftSerial pss(D0, D1);
// uCamIII<ParticleSoftSerial> ucam(pss);

uint8_t imageBuffer[40960];                                         // max raw image 160x128x2byte
int     imageSize    = 0;
int     imageType    = uCamIII_SNAP_RAW;
int     imageWidth   = 0;
int     imageHeight  = 0;
int     imagePxDepth = 0;
char    lIP[16];

void setup() {
  Particle.function("snap", takeSnapshot);
  pinMode(D7, OUTPUT);
  ucam.init(115200);

#if Wiring_WiFi
  strncpy(lIP, String(WiFi.localIP()), sizeof(lIP));
  Particle.variable("IP", lIP);

  webserver.setDefaultCommand(&defaultCmd);
  webserver.addCommand("index.html", &defaultCmd);
  webserver.addCommand("image.dat", &imageCmd);
  webserver.begin();
#endif
}

void loop() {
#if Wiring_WiFi
  char buff[64];
  int len = 64;
  webserver.processConnection(buff, &len);
#endif
}

int takeSnapshot(String format) 
{
  Log.trace(__FUNCTION__); 

  if (format.equalsIgnoreCase("JPG") || format.equalsIgnoreCase("JPEG"))
    return takePicture(uCamIII_SNAP_JPEG, uCamIII_COMP_JPEG, uCamIII_640x480, 
                       uCamIII_DEFAULT, uCamIII_DEFAULT, uCamIII_DEFAULT, callback);
  else if (format.equalsIgnoreCase("RGB16"))
    return takePicture(uCamIII_SNAP_RAW, uCamIII_RAW_16BIT_RGB565, uCamIII_160x120, 
                       uCamIII_DEFAULT, uCamIII_DEFAULT, uCamIII_DEFAULT, callback);
  else if (format.equalsIgnoreCase("UYVY16") || format.equalsIgnoreCase("CrYCbY16"))
    return takePicture(uCamIII_SNAP_RAW, uCamIII_RAW_16BIT_CRYCBY, uCamIII_160x120, 
                       uCamIII_DEFAULT, uCamIII_DEFAULT, uCamIII_DEFAULT, callback);
  else // default to "GRAY8"
    return takePicture(uCamIII_SNAP_RAW, uCamIII_RAW_8BIT, uCamIII_160x120, 
                       uCamIII_DEFAULT, uCamIII_DEFAULT, uCamIII_DEFAULT, callback);
}

long takePicture(uCamIII_SNAP_TYPE snap, uCamIII_IMAGE_FORMAT fmt, uCamIII_RES res, 
                 uCamIII_CBE contrast, uCamIII_CBE brightness, uCamIII_CBE exposure,
                 uCamIII_callback callback)
{
  uCamIII_PIC_TYPE type = uCamIII_TYPE_SNAPSHOT;    // default for the demo 
                                                    // alternative: _TYPE_RAW & _TYPE_JPEG
  digitalWrite(D7, HIGH);

  ucam.hardReset();
  
  if (!ucam.sync()) return -1;
  
  if (!ucam.setImageFormat(fmt, res)) return -2;
  
  ucam.setCBE(contrast, brightness, exposure);

  if (type == uCamIII_TYPE_SNAPSHOT)
    if (!ucam.takeSnapshot(snap)) return -3;

  if (fmt == uCamIII_COMP_JPEG) 
    if (!ucam.setPackageSize(512)) return -4;

  // if we made it to here, we can set the global image variables accordingly 
  imageType = snap;             
  
  switch(res) 
  {
    case uCamIII_80x60:
      imageWidth  = 80;
      imageHeight = 60;
      break;
    case uCamIII_160x120:          
      imageWidth  = 160;
      imageHeight = 120;
      break;
    case uCamIII_320x240:
      imageWidth  = 320;
      imageHeight = 240;
      break;
    case uCamIII_640x480:
      imageWidth  = 640;
      imageHeight = 480;
      break;
    case uCamIII_128x96:
      imageWidth  = 128;
      imageHeight = 96;
      break;
    case uCamIII_128x128:
      imageWidth  = 128;
      imageHeight = 128;
      break;
    default:
      imageWidth  = 80;
      imageHeight = 60;
      break;
  }

  switch (fmt)
  {
    case uCamIII_COMP_JPEG:
      imagePxDepth = 24;
      break;
    case uCamIII_RAW_16BIT_RGB565:
    case uCamIII_RAW_16BIT_CRYCBY:
      imagePxDepth = 16;
      break;
    case uCamIII_RAW_8BIT:
    defaut:
      imagePxDepth = 8;
      break;
  }
  
  if (imageSize = ucam.getPicture(type)) 
  {
    Log.info("\r\nImageSize: %d", imageSize);
    if (imageType == uCamIII_SNAP_JPEG)
      for (int received = 0, chunk = 0; (received < imageSize) && (chunk = ucam.getJpegData(&imageBuffer[received], 512, callback)); received += chunk);
    else 
      ucam.getRawData(imageBuffer, imageSize, callback);
      
    digitalWrite(D7, LOW);
    return imageSize;
  }

  return 0;
}

int callback(uint8_t *buf, int len, int id)
{
  Log.info("Package %d (%d Byte)", id, len);
  return Serial.write(buf, len);
}


#if Wiring_WiFi
void defaultCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  Log.trace(__FUNCTION__); 

  int snap   = -1;  // uCamIII_SNAP_TYPE
  int fmt    = -1;  // uCamIII_IMAGE_FORMAT 
  int res    = -1;  // uCamIII_RES          
  int rawRes = -1;
  int jpgRes = -1;
  int contr  = 0;
  int bright = 0;
  int expose = 0;
  
  server.httpSuccess();

  if (type == WebServer::HEAD)
    return;

  server.printP(Page_start);
  server.printP(camBody);

  if (strlen(url_tail))
  {
    URLPARAM_RESULT rc;
    char name[NAMELEN];
    char value[VALUELEN];
    char strScript[strlen((const char*)script)];
    
    while (strlen(url_tail))
    {
      rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
      if (rc != URLPARAM_EOS)
      {
        if (!strcmp(name, COLOR_TYPE))
        {
          fmt = atoi(value);
          snap = (fmt == uCamIII_COMP_JPEG) ? uCamIII_SNAP_JPEG : uCamIII_SNAP_RAW;
        }
        else if (!strcmp(name, RAW_RES))
          rawRes = atoi(value);           
        else if (!strcmp(name, JPG_RES))
          jpgRes = atoi(value);           
        else if (!strcmp(name, CONTRAST))
          contr = atoi(value);           
        else if (!strcmp(name, BRIGHTNESS))
          bright = atoi(value);           
        else if (!strcmp(name, EXPOSURE))
          expose = atoi(value);           
      }
    }
    snprintf(strScript, sizeof(strScript), (const char*)script, fmt, rawRes, jpgRes, contr, bright, expose);
    server.print(strScript);
  }

  server.printP(Page_end);

  res = (fmt == uCamIII_COMP_JPEG) ? jpgRes : rawRes;
  if (snap >= 0 && fmt > 0 && res > 0)
    takePicture((uCamIII_SNAP_TYPE)snap, (uCamIII_IMAGE_FORMAT)fmt, (uCamIII_RES)res, 
                (uCamIII_CBE)contr, (uCamIII_CBE)bright, (uCamIII_CBE)expose);
}

void imageCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  Log.trace(__FUNCTION__); 
  if (!imageSize) return;
  
  if (imageType == uCamIII_SNAP_RAW) 
  {
    Log.trace("BMP %dx%dx%d", imageWidth, imageHeight, imagePxDepth);
    sendBmpHeader(server, imageWidth, imageHeight, imagePxDepth);
  }

  server.write(imageBuffer, imageSize);
}

struct BITMAPFILEHEADER 
{
  uint16_t   bfType;            // file type BM (0x424d)                        [  0]
  uint32_t   bfSize;            // file size all together                       [  2]
  uint16_t   bfReserved1;       //                                              [  6]
  uint16_t   bfReserved2;       //                                              [  8]
  uint32_t   bfOffBits;         // offset of actual image data                  [ 10]
} __attribute__((__packed__));  //                                              ( 14)

// https://msdn.microsoft.com/en-us/library/windows/desktop/dd183381(v=vs.85).aspx
struct BITMAPV5HEADER
{
  // --------------------------------------------------------------------------
  // legacy BITMAPINFOHEADER: 
  uint32_t Size;              // Size of this header in bytes               [  0][ 14]
  int32_t  Width;             // Image width in pixels                      [  4][ 18]
  int32_t  Height;            // Image height in pixels                     [  8][ 22]
  uint16_t Planes;            // Number of color planes                     [ 12][ 26]
  uint16_t BitsPerPixel;      // Number of bits per pixel                   [ 14][ 28]
  uint32_t Compression;       // Compression methods used                   [ 16][ 30]
  uint32_t SizeOfBitmap;      // Size of bitmap in bytes                    [ 20][ 34]
  int32_t  HorzResolution;    // Horizontal resolution in pixels per meter  [ 24][ 38] 
  int32_t  VertResolution;    // Vertical resolution in pixels per meter    [ 28][ 42]
  uint32_t ColorsUsed;        // Number of colors in the image              [ 32][ 46]
  uint32_t ColorsImportant;   // Minimum number of important colors         [ 36][ 50]
  // --------------------------------------------------------------------   ( 40)( 54)
  // Fields added for V2:
  uint32_t RedMask;           // Mask identifying bits of red component     [ 40][ 54]
  uint32_t GreenMask;         // Mask identifying bits of green component   [ 44][ 58]
  uint32_t BlueMask;          // Mask identifying bits of blue component    [ 48][ 62]
  // --------------------------------------------------------------------   ( 52)( 66)
  // Fields added for V3:
  uint32_t AlphaMask;         // Mask identifying bits of alpha component   [ 52][ 66]
/*// --------------------------------------------------------------------   ( 56)( 70)
  // Fields added for V4:
  uint32_t CSType;            // Color space type                           [ 56][ 70]
  int32_t  RedX;              // X coordinate of red endpoint               [ 60][ 74]
  int32_t  RedY;              // Y coordinate of red endpoint               [ 64][ 78]
  int32_t  RedZ;              // Z coordinate of red endpoint               [ 68][ 82]
  int32_t  GreenX;            // X coordinate of green endpoint             [ 72][ 86]
  int32_t  GreenY;            // Y coordinate of green endpoint             [ 76][ 90]
  int32_t  GreenZ;            // Z coordinate of green endpoint             [ 80][ 94]
  int32_t  BlueX;             // X coordinate of blue endpoint              [ 84][ 98]
  int32_t  BlueY;             // Y coordinate of blue endpoint              [ 88][102]
  int32_t  BlueZ;             // Z coordinate of blue endpoint              [ 92][106]
  uint32_t GammaRed;          // Gamma red coordinate scale value           [ 96][110]
  uint32_t GammaGreen;        // Gamma green coordinate scale value         [100][114]
  uint32_t GammaBlue;         // Gamma blue coordinate scale value          [104][118]
  // --------------------------------------------------------------------   (108)(122)
  // Fields added for V5:
  uint32_t Intent;            //                                            [108][122]
  uint32_t ProfileData;       //                                            [112][126]
  uint32_t ProfileSize;       //                                            [116][130]
  uint32_t Reserved;          //                                            [120][134]
*/
} __attribute__((__packed__));

void sendBmpHeader(WebServer &server, int width, int height, int bits, int encoding, int padding)  
{  
  Log.trace(__FUNCTION__); 

  BITMAPFILEHEADER  bmpFileHeader;
  BITMAPV5HEADER    bmpInfoHeader;
  uint32_t          headerSize      = sizeof(bmpFileHeader) 
                                    + sizeof(bmpInfoHeader);
  uint32_t          paletteSize     = (bits <= 8) ? (1 << (bits+2)) : 0;    // optional palette
  uint32_t          pixelDataSize   = height * ((width * (bits / 8)) + padding);  

  /// ToDo: where do we set encoding R5G6B5 vs. CrYCbY?

  memset(&bmpFileHeader, 0, sizeof(bmpFileHeader));
  bmpFileHeader.bfType              = 0x4D42;                               // magic number "BM"
  bmpFileHeader.bfSize              = headerSize 
                                    + paletteSize
                                    + pixelDataSize;
  //bmpFileHeader.bfReserved1         =
  //bmpFileHeader.bfReserved2         = 0;
  bmpFileHeader.bfOffBits           = headerSize
                                    + paletteSize;

  memset(&bmpInfoHeader, 0, sizeof(bmpInfoHeader));
  bmpInfoHeader.Size                = sizeof(bmpInfoHeader);  
  bmpInfoHeader.Width               = width;                               
  bmpInfoHeader.Height              = -height;                              // negative height flips vertically
                                                                            // so image isn't upside down 
                                                                            // but Google Chorme doesn't support that!!! 
  bmpInfoHeader.Planes              = 1;  
  bmpInfoHeader.BitsPerPixel        = bits;  
  //bmpInfoHeader.Compression         = 0; 
  bmpInfoHeader.SizeOfBitmap        = 0x03;                                 // BI_RGB (0), BI_BITFIELD (3)
  bmpInfoHeader.HorzResolution      = 0x1000; 
  bmpInfoHeader.VertResolution      = 0x1000; 
  bmpInfoHeader.ColorsUsed          =     
  bmpInfoHeader.ColorsImportant     = (bits <= 8) ? (1 << bits) : 0;        // <= 8bit use color palette
  if (bits == 16)
  {  
    bmpInfoHeader.RedMask           = 0x0000f800;
	bmpInfoHeader.GreenMask         = 0x000007e0;
	bmpInfoHeader.BlueMask          = 0x0000001f;
  }

  server.write((uint8_t*)&bmpFileHeader, sizeof(bmpFileHeader));
  server.write((uint8_t*)&bmpInfoHeader, sizeof(bmpInfoHeader));

  for (int c = 0; c < bmpInfoHeader.ColorsUsed; c++)                         // grayscale color table
  {
    uint8_t color = 255 * c / (bmpInfoHeader.ColorsUsed - 1);
    uint8_t grey[4] = { color, color, color, 0xFF };
    server.write(grey, sizeof(grey));  
  }

  if (bits == 16) 
  {
    for (int i = 0; i < imageSize; i += 2)                          // raw image comes big-endian and upside down from cam,
    {                                                               // this block corrects endianness
      uint8_t dmy = imageBuffer[i];
      imageBuffer[i] = imageBuffer[i+1];
      imageBuffer[i+1] = dmy;
    }
  }
}  
#endif
