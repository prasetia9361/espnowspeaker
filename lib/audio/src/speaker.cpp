#include "speaker.h"
#include "soc/i2s_reg.h"
#include <esp_log.h>

static const char *TAG = "OUT";
speaker::speaker(int size){
    rawSamplesSize = size;
    frames = (int16_t *)malloc(sizeof(int16_t) * rawSamplesSize);
}

speaker::~speaker(){
    free(frames);
}

void speaker::startSpeaker(uint16_t sampleRate){
i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 1)
        .sample_rate = sampleRate,
#else
        .sample_rate = (int)sample_rate,
#endif
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT, //  I2S_CHANNEL_FMT_RIGHT_LEFT
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 2, 0)
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
#else
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S),
#endif
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 2,
        .dma_buf_len = 128,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 1)
        .mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT,  // Unused
        .bits_per_chan = I2S_BITS_PER_CHAN_DEFAULT   // Use bits per sample
#endif
    };
    i2s_driver_install(i2sPort, &i2s_config, 0, NULL);
    i2s_set_pin(i2sPort, &i2sSpeakerPins);
    i2s_zero_dma_buffer(i2sPort);

    i2s_start(i2sPort);
}

void speaker::stopAudio(){
    i2s_stop(i2sPort);
    i2s_driver_uninstall(i2sPort);
}

void speaker::write(int16_t *samples, int count){
    int sampleIndex = 0;
    static int16_t prev_sample = 0;
    const float alpha = 0.1; 
    while (sampleIndex < count)
    {
        int samplestoSend = 0;
        for (int i = 0; i < rawSamplesSize && sampleIndex < count; i++){
                int sample = processSample(samples[sampleIndex]);
                frames[i] = sample;
                // frames[i + 1] = sample;

                // Serial.println(frames[i]);
                // frames[i] = (sample > INT16_MAX) ? INT16_MAX : (sample < -INT16_MAX) ? -INT16_MAX : (int16_t)sample;

                samplestoSend++;
                sampleIndex++;
        }
        size_t bytes_written = 0;
        i2s_write(i2sPort, frames, samplestoSend * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        if (bytes_written != samplestoSend * sizeof(int16_t))
        {
            ESP_LOGE(TAG, "Did not write all bytes");
        }
    }
}