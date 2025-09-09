#include "button.h"

void button::begin() {
    pinMode(_pin, INPUT_PULLUP);
}

void button::onDoubleClick() {
    mode = true;
    // Serial.println("Double Click Detected!"); // Untuk debug
}

void button::onLongPress() {
    removeData = true;
    // Serial.println("Long Press Detected!"); // Untuk debug
}

void button::tick() {
    // 1. Baca status pin
    int reading = digitalRead(_pin);

    // 2. Debouncing
    // Jika sinyal berubah, reset timer debounce
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }

    // Jika sinyal sudah stabil selama periode debounceDelay
    if ((millis() - lastDebounceTime) > debounceDelay) {
        // Jika status tombol memang berubah
        if (reading != buttonState) {
            buttonState = reading;

            // Jika tombol baru saja DITEKAN (transisi dari HIGH ke LOW)
            if (buttonState == LOW) {
                isLongPress = false;
                longPressTime = millis(); // Mulai hitung waktu untuk long press
            }
            // Jika tombol baru saja DILEPAS (transisi dari LOW ke HIGH)
            else {
                // Jika tombol dilepas sebelum dianggap long press
                if (!isLongPress) {
                    clickCount++; // Tambah hitungan klik
                    lastClickTime = millis(); // Catat waktu klik
                }
            }
        }
    }

    // Simpan status bacaan terakhir
    lastButtonState = reading;

    // 3. Cek event Long Press
    // Jika tombol sedang ditekan dan belum terdeteksi long press
    if (buttonState == LOW && !isLongPress) {
        if ((millis() - longPressTime) > longPressDelay) {
            onLongPress();
            isLongPress = true; // Tandai agar tidak terpicu lagi
        }
    }

    // 4. Cek event Double Click
    // Jika ada klik yang tercatat dan waktu tunggu double click sudah lewat
    if (clickCount > 0 && (millis() - lastClickTime) > doubleClickTimeout) {
        if (clickCount == 2) {
            onDoubleClick();
        }
        // Reset hitungan klik setelah timeout, baik itu 1 atau 2 klik
        clickCount = 0;
    }
}