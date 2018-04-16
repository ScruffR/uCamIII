#include <TCPClientX.h>

size_t TCPClientX::write(const uint8_t *buffer, size_t size)
{
  size_t pos; 
  for (pos = 0; pos < size;)
  {
    if (size - pos < chunkSize) chunkSize = size - pos;
    int sent = TCPClient::write(&buffer[pos], chunkSize);
    if (sent == chunkSize)
      pos += chunkSize;
    else if (sent == -16) 
      for (uint32_t ms = millis(); millis() - ms < flushDelayTime; Particle.process());
  }
  return pos;
}
