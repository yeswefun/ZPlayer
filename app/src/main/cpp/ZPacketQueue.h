//
// Created by lin on 2021/11/14.
//

#ifndef P00_ZPACKETQUEUE_H
#define P00_ZPACKETQUEUE_H

extern "C" {
#include "libavformat/avformat.h"
#include <pthread.h>
}

#include <queue>
#include "ZConstants.h"

class ZPacketQueue {
public:
    std::queue<AVPacket*> *packetQueue = NULL;
    pthread_mutex_t packetMutex;
    pthread_cond_t packetCond;

    ZPacketQueue();
    ~ZPacketQueue();

    void push(AVPacket *packet);
    AVPacket *pop();
    void clear();
};


#endif //P00_ZPACKETQUEUE_H
