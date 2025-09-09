#ifndef STORAGE_H
#define STORAGE_H

#include <SPIFFS.h>
#include <ArduinoJson.h>

#include <freertos/FreeRTOS.h>
#include "FS.h"

class storage {
private:
    typedef struct config {
        uint8_t macAddress[6] = {0,0,0,0,0,0};
        uint8_t macAddress1[6] = {0,0,0,0,0,0};
        int32_t modeArray[9] = {0,0,0,0,0,0,0,0,0};
        char nameDevice1[12] = "";
        char nameDevice2[12] = "";
        int dataInt;
    } config;
    config configData;

    SemaphoreHandle_t semaphore;
    
public:
    storage();
    ~storage();

    uint8_t *getMac(){return configData.macAddress;}
    uint8_t *getMac1(){return configData.macAddress1;}
    int32_t *readModeTones(){return configData.modeArray;}
    char *device1(){return configData.nameDevice1;}
    char *device2(){return configData.nameDevice2;}
    int getVolume(){return configData.dataInt;}

    void init();
    void writeMode(const uint8_t *bufferMode, int count);
    void writeMode(const int32_t* bufferMode, int count);
    int* readMode();
    void saveVolume(int data);
    void writeMacAddress(const uint8_t *mac, int count);
    void writeMacAddress(const uint8_t *mac, const char *device, int count);
    void deleteAddress();
    bool hapusAlamat(const char *deviceName);
};
#endif