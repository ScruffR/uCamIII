#pragma once

#include <Particle.h>

class TCPClientX : public TCPClient {

public:
    TCPClientX(size_t chunk = 512, uint32_t flushDelay = 100) 
    : TCPClient(), chunkSize(chunk), flushDelayTime(flushDelay) { }

    virtual size_t write(const uint8_t *buffer, size_t size);

    using Print::write;

private:
    size_t   chunkSize;
    uint32_t flushDelayTime;

};
