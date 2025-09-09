#include <Arduino.h>
#include <driver/gpio.h>

#include "commEspNow.h"
#include "speaker.h"
#include "sirine.h"
#include "buffer.h"
#include "storage.h"
#include "button/button.h"



// sample rate for the system
// #define SAMPLE_RATE 22050
#define SAMPLE_RATE 44100 

#define BINDING_BUTTON GPIO_NUM_0

// On which wifi channel (1-11) should ESP-Now transmit? The default ESP-Now
// channel on ESP32 is channel 1
#define ESP_NOW_WIFI_CHANNEL 1

#define TRANSPORT_HEADER_SIZE 0
extern uint8_t transportHeader[TRANSPORT_HEADER_SIZE];

class receiverTask{
private:
    commEspNow *mCommunication;
    storage *mMemory; 
    speaker *mOutput;
    sirine *mSirine;
    Buffer *outBuffer;
    button *mButton; 

    int16_t *samples = nullptr;
    
    int siren = 0;
    int mode =0;
    int volSpeaker = 11;

    unsigned long currentTime;
    void clearSample();
public:
    receiverTask();
    ~receiverTask();
    void begin();
    void communication();
    void processData();
};