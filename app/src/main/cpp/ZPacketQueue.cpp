//
// Created by lin on 2021/11/14.
//

#include "ZPacketQueue.h"

ZPacketQueue::ZPacketQueue() {
    packetQueue = new std::queue<AVPacket *>();
    pthread_mutex_init(&packetMutex, NULL);
    pthread_cond_init(&packetCond, NULL);
}

ZPacketQueue::~ZPacketQueue() {
    if (packetQueue) {
        clear();
        delete packetQueue;
        packetQueue = NULL;
    }
    pthread_mutex_destroy(&packetMutex);
    pthread_cond_destroy(&packetCond);
}

void ZPacketQueue::push(AVPacket *packet) {
    pthread_mutex_lock(&packetMutex);
//    while (packetQueue->full()) {
//        pthread_cond_wait(&packetCond, &packetMutex);
//    }
    packetQueue->push(packet);
    pthread_cond_signal(&packetCond);
    pthread_mutex_unlock(&packetMutex);
}

AVPacket *ZPacketQueue::pop() {
    AVPacket *packet = NULL;
    pthread_mutex_lock(&packetMutex);
    while (packetQueue->empty()) {
        pthread_cond_wait(&packetCond, &packetMutex);
    }
    packet = packetQueue->front();
    packetQueue->pop();
    pthread_mutex_unlock(&packetMutex);
    return packet;
}

/*
 * 将队列中的所有packet都释放掉
 */
void ZPacketQueue::clear() {
    LOGE("ZPacketQueue: clear()");
}
