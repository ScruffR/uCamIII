/* *************************************************************************************

http://www.4dsystems.com.au/productpages/uCAM-III/downloads/uCAM-III_datasheet_R_1_0.pdf

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

uCamIII<USARTSerial> ucam(Serial1, A0);
// or
// uCamIII<ParticleSoftSerial> ucam(new ParticleSoftSerial(D0, D1));
// or
// ParticleSoftSerial pss(D0, D1);
// uCamIII<ParticleSoftSerial> ucam(pss);

//SerialLogHandler traceLog(LOG_LEVEL_TRACE);

uint8_t jpgBuffer[512];
uint8_t rawBuffer[32768];

int     imageSize = 0;
int     imageType = uCamIII_SNAP_RAW;

void setup() {
  Particle.function("snap", takeSnapshot);
  pinMode(D7, OUTPUT);
  ucam.init(115200);
}

void loop() {
  if (!imageSize) return;
  Log.info("\r\nImageSize: %d", imageSize);
  if (imageType == uCamIII_SNAP_JPEG)
    for (int received = 0, chunk = 0; (received < imageSize) && (chunk = ucam.getJpegData(jpgBuffer, sizeof(jpgBuffer), callback)); received += chunk);
  else 
    ucam.getRawData(rawBuffer, imageSize, callback);

  digitalWrite(D7, LOW);
  imageSize = 0;
}

long takeSnapshot(String format) 
{
  uCamIII_PIC_TYPE     type = uCamIII_TYPE_SNAPSHOT; 
  uCamIII_SNAP_TYPE    snap = uCamIII_SNAP_RAW;
  uCamIII_IMAGE_FORMAT fmt  = uCamIII_RAW_8BIT;
  uCamIII_RES          res  = uCamIII_160x120;
  digitalWrite(D7, HIGH);
 
  ucam.hardReset();
  
  if (!ucam.sync()) return -1;
  
  if (format.equalsIgnoreCase("JPG") || format.equalsIgnoreCase("JPEG"))
  {
    //type = uCamIII_TYPE_JPEG; // only for immediate pic like video stream
    snap = uCamIII_SNAP_JPEG;
    fmt  = uCamIII_COMP_JPEG;
    res  = uCamIII_640x480;
  }
  else if (format.equalsIgnoreCase("RAW16"))
  {
    fmt = uCamIII_RAW_16BIT_R5G6B5;
  }
  if (!ucam.setImageFormat(fmt, res)) return -2;

  if (type == uCamIII_TYPE_SNAPSHOT)
    if (!ucam.takeSnapshot(snap)) return -3;

  if (fmt == uCamIII_COMP_JPEG) 
    if (!ucam.setPackageSize(sizeof(jpgBuffer))) return -4;

  imageType = snap;
  
  if (imageSize = ucam.getPicture(type))
    return imageSize;

  return 0;
}

int callback(uint8_t *buf, int len, int id)
{
  Log.info("Package %d (%d Byte)", id, len);
  return Serial.write(buf, len);
}
