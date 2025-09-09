#pragma once 

#include <Arduino.h>
#include <driver/i2s.h>

#define SAMPLE_RATE 44100
#define WAVE_TABLE_SIZE 256
#define N_VALUE 0.4f
#define M_VALUE 0.4f

class sirine
{
private:
    //tone controll
    uint8_t volume;       // Volume 0-100%
    uint8_t mode;      
    uint8_t envelopeVolume;  // Envelope volume untuk Mode 4
    uint32_t phaseAccumulator;

    // Wave table dan phase step
    int16_t waveTable[WAVE_TABLE_SIZE];
    int16_t combineTable[WAVE_TABLE_SIZE];
    int16_t airhornTable[WAVE_TABLE_SIZE];
    int16_t toneTable[WAVE_TABLE_SIZE];
    volatile uint32_t phaseStep = 0;

    i2s_port_t i2sPort = I2S_NUM_0;
    const uint8_t modeTableMap(int mode);
public:
    sirine();
    ~sirine();
    //Sirine
    void generateWaveTable();
    void generateI2sTone(uint8_t mode);
    void generateSineWave(int vol);
    void cleanBuffer();
};
