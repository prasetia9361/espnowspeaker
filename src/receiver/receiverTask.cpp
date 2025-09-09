#include "receiverTask.h"

uint8_t transportHeader[TRANSPORT_HEADER_SIZE] = {};


receiverTask::receiverTask()
{
    mOutput = new speaker(128);// 256
    mSirine = new sirine();
    outBuffer = new Buffer(300 * 16);
    mMemory = new storage();
    mCommunication = new commEspNow(outBuffer, mMemory, ESP_NOW_WIFI_CHANNEL);
    mCommunication->setHeader(TRANSPORT_HEADER_SIZE, transportHeader);
    mButton = new button(BINDING_BUTTON); 
    
    // Alokasi memory yang aman untuk samples
    samples = (int16_t *)malloc(sizeof(int16_t) * 128);
    if (!samples) {
        Serial.println("Error: Failed to allocate memory for samples");
    }
}

receiverTask::~receiverTask()
{
    if (mCommunication) delete mCommunication;
    if (mMemory) delete mMemory;
    if (mOutput) delete mOutput;
    if (mButton) delete mButton;
}

void receiverTask::begin(){
    if (I2S_SPEAKER_SD_PIN != -1)
    {
        pinMode(I2S_SPEAKER_SD_PIN, OUTPUT);
    }

    Serial.print("My IDF Version is: ");
    Serial.println(esp_get_idf_version());
    
    // Delay untuk memastikan sistem stabil
    delay(200);
    
    // Inisialisasi komunikasi dan komponen lain
    mMemory->init(); 
    delay(100);

    mSirine->generateWaveTable();
    delay(50);
    mOutput->startSpeaker(SAMPLE_RATE); 
    delay(100); // Delay untuk memastikan I2S stabil

    mButton->begin(); 
    outBuffer->flush();
    Serial.println("Application started");
}

void receiverTask::communication(){
    if (!mCommunication->begin()) {
        Serial.println("Komunikasi gagal dimulai!");
        return;
    }

    for (;;)
    {
        if (mButton->getMode()){
            Serial.println("Proses binding dimulai");
            mCommunication->binding();
            mButton->setMode(false); 
        }

        if (mButton->getRemove()) 
        {
            mMemory->deleteAddress(); 
            mButton->setRemove(false); 
        }

        int newSiren = mCommunication->getButtonValue();
        if (siren != newSiren) {
            siren = newSiren;
        }
        int newMode = mCommunication->getMode();
        if (mode != newMode) {
            mode = newMode;
        }
        mButton->tick();
        vTaskDelay(5);
    }
}

void receiverTask::clearSample(){
    if (samples) {
        free(samples);
    }
}

void receiverTask::processData(){
    while (true)
    {
        unsigned long start_time = 0;
        switch (mode) {
            case 1: {
                while (siren >= 1) {
                    mSirine->generateI2sTone(siren);
                    mSirine->generateSineWave(100); // mMemory->getVolume()
                    if (siren == 0) {
                        mSirine->cleanBuffer();
                        break;
                    }
                    // vTaskDelay(1);
                }
                break;
            }
            default: {
                if (!samples) {
                    Serial.println("Error: samples buffer is null");
                    return;
                }
                
                start_time = millis();
                while (millis() - start_time < 1000 || !outBuffer->getBuffer())
                {
                    if (I2S_SPEAKER_SD_PIN != -1)
                    {
                        digitalWrite(I2S_SPEAKER_SD_PIN, HIGH);
                    }
                    volSpeaker = map(100, 0, 100, 0, 10);//mMemory->getVolume()
                    outBuffer->removeBuffer(samples, 128, volSpeaker);
                    mOutput->write(samples, 128);
                    if (I2S_SPEAKER_SD_PIN != -1)
                    {
                        digitalWrite(I2S_SPEAKER_SD_PIN, LOW);
                    }
                    yield();
                }
                break;
            }
        }
        vTaskDelay(5);
    }
    clearSample();
}