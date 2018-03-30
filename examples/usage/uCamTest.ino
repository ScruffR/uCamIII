#include "uCamIII.h"

uCamIII<USARTSerial> ucam(Serial1, A0);
// or
// uCamIII<ParticleSoftSerial> ucam(new ParticleSoftSerial(D0, D1));
// or
// ParticleSoftSerial pss(D0, D1);
// uCamIII<ParticleSoftSerial> ucam(pss);

//SerialLogHandler traceLog(LOG_LEVEL_TRACE);


uint8_t buffer[512];
int     imageSize = 0;

void setup() {
  Particle.function("snap", takeSnapshot);
  pinMode(D7, OUTPUT);
  ucam.init(115200);
}

void loop() {
  if (!imageSize) return;
  Log.info("\r\nImageSize: %d", imageSize);
  for (int received = 0, chunk = 0; (received < imageSize) && (chunk = ucam.getData(buffer, sizeof(buffer), callback)); received += chunk);
  digitalWrite(D7, LOW);
  imageSize = 0;
}

int takeSnapshot(String format) 
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
    res  = uCamIII_320x240;
  }
  else if (format.equalsIgnoreCase("RAW16"))
  {
    fmt = uCamIII_RAW_16BIT_R5G6B5;
  }
  if (!ucam.setImageFormat(fmt, res)) return -2;

  if (type == uCamIII_TYPE_SNAPSHOT)
    if (!ucam.takeSnapshot(snap)) return -3;

  if (fmt == uCamIII_COMP_JPEG) 
    if (!ucam.setPackageSize(sizeof(buffer))) return -4;
  
  if (imageSize = ucam.getPicture(type))
    return imageSize;

  return 0;
}

int callback(uint8_t *buf, int len, int id)
{
  Log.info("Package %d (%d Byte)", id, len);
  return Serial.write(buf, len);
}
