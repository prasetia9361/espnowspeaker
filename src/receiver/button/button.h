#pragma once
#include "Arduino.h" // Diperlukan untuk millis(), digitalRead(), dll.

class button
{
private:
    // Pin & State Flags
    int _pin;
    bool mode;
    bool removeData;

    // Variabel untuk State Machine & Timing manual
    int buttonState;
    int lastButtonState;

    unsigned long lastDebounceTime;
    unsigned long debounceDelay;

    unsigned long longPressTime;
    unsigned long longPressDelay;
    bool isLongPress;

    unsigned long lastClickTime;
    unsigned long doubleClickTimeout;
    byte clickCount;

    // Fungsi privat untuk aksi
    void onDoubleClick();
    void onLongPress();

public:
    button(int pin) : _pin(pin) {
        mode = false;
        removeData = false;

        // Inisialisasi variabel status
        lastButtonState = HIGH; // Asumsi tombol tidak ditekan (pull-up)
        buttonState = HIGH;
        lastDebounceTime = 0;
        longPressTime = 0;
        lastClickTime = 0;
        clickCount = 0;
        isLongPress = false;

        // Nilai waktu (bisa diubah sesuai kebutuhan)
        debounceDelay = 50;      // 50 ms
        longPressDelay = 1000;   // 1 detik
        doubleClickTimeout = 300; // 300 ms
    }

    void begin();
    bool getMode() { return mode; }
    bool setMode(bool value) {
        mode = value;
        return mode;
    }
    bool getRemove() { return removeData; }
    bool setRemove(bool value) {
        removeData = value;
        return removeData;
    }
    void tick();
};