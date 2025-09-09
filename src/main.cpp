#include <Arduino.h>
#include "receiver/receiverTask.h"
receiverTask *receiver;

void appCore0(void *param);
void appCore1(void *param);

void setup(){
    Serial.begin(115200);

    receiver = new receiverTask();

    receiver->begin();

    TaskHandle_t handleCore0;
    xTaskCreatePinnedToCore(
        appCore0,
        "appCore0",
        32768,
        NULL,
        1,
        &handleCore0,
        0
    );

    TaskHandle_t handleCore1;
    xTaskCreatePinnedToCore(
        appCore1,
        "appCore1",
        32768,
        NULL,
        1,
        &handleCore1,
        1
    );
}

void loop(){
    vTaskDelay(pdMS_TO_TICKS(1000));
}

void appCore0(void *param){
    receiver->communication();
}

void appCore1(void *param){
    receiver->processData();
}
