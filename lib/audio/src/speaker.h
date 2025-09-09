#pragma once

#include <Arduino.h>
#include <driver/i2s.h>

// speaker settings
#define USE_I2S_SPEAKER_OUTPUT
#define I2S_SPEAKER_SERIAL_CLOCK 7
#define I2S_SPEAKER_LEFT_RIGHT_CLOCK 6
#define I2S_SPEAKER_SERIAL_DATA 8
// Shutdown line if you have this wired up or -1 if you don't
#define I2S_SPEAKER_SD_PIN -1

class speaker
{
private:
    int16_t *frames;
    int rawSamplesSize;

    i2s_pin_config_t i2sSpeakerPins = {
        .bck_io_num = I2S_SPEAKER_SERIAL_CLOCK,
        .ws_io_num = I2S_SPEAKER_LEFT_RIGHT_CLOCK,
        .data_out_num = I2S_SPEAKER_SERIAL_DATA,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_port_t i2sPort = I2S_NUM_0;
public:
    speaker(int size);
    ~speaker();
    // speaker
    void startSpeaker(uint16_t sampleRate);
    void stopAudio();
    int16_t processSample(int16_t sample){return sample;}
    void write(int16_t *samples, int count);
};