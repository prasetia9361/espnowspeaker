#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "storage.h"
#include "buffer.h"

const int maxEspNowPacketSize = 127;

class commEspNow {
private:
    typedef struct message {
        uint8_t buffer[maxEspNowPacketSize];
        char data[20] = "";
        int dataLen;
    } message;

    message messageData;
    Buffer* audioBuffer;
    storage* memoryStorage;
    uint8_t wifiChannel;
    int bufferSize;
    int index;
    int headerSize;
    int lastData;
    int buttonValue = 0;
    const uint8_t* mac = nullptr;

public:
    commEspNow(Buffer* audioBuffer, storage* memoryStorage, uint8_t wifiChannel);
    ~commEspNow();
    bool begin();
    bool addPeer();
    bool addPeer(const uint8_t* _mac);
    void sendData();
    
    // Fungsi audio
    void addSample(int16_t sample);
    void flush();

    // sending data 
    void sendDataInt(int data, const char *header);
    void sendModeSiren(const uint8_t *modelBuffer);
    // recieve data
    int getButtonValue() { return buttonValue; }
    int getMode() {return messageData.dataLen;}
    
    // Fungsi binding
    void statusBinding();
    bool binding();
    
    // Header settings
    int setHeader(const int headerSize, const uint8_t* header);
    
    // Friend fungsi untuk callback
    friend void receiverCallback(const uint8_t* macAddr, const uint8_t* data, int dataLen);
    friend void transmitterCallback(const uint8_t* macAddr, const uint8_t* data, int dataLen);
    friend void displayCallback(const uint8_t* macAddr, const uint8_t* data, int dataLen);
};

// Callback function declarations
void receiverCallback(const uint8_t* macAddr, const uint8_t* data, int dataLen);