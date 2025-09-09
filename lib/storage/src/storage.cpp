#include "storage.h"

storage::storage() {
    // Tidak perlu inisialisasi semaphore
}

storage::~storage() {
    // Tidak perlu hapus semaphore
}

void storage::init() {
    Serial.println("[DEBUG] Memulai inisialisasi storage");
    
    // Tidak perlu ambil semaphore

    if (!SPIFFS.begin(true)) {
        Serial.println("[ERROR] SPIFFS init failed");
        return;
    }
    Serial.println("[DEBUG] SPIFFS berhasil diinisialisasi");

    // Handle config.json
    if (SPIFFS.exists("/config.json")) {
        Serial.println("[DEBUG] File config.json ditemukan");
        File file = SPIFFS.open("/config.json", FILE_READ);
        if (file) {
            size_t size = file.size();
            Serial.printf("[DEBUG] Ukuran file config.json: %d bytes\n", size);
            
            if (size > 0 && size < 512) {
                String content = file.readString();
                file.close();
                Serial.println("[DEBUG] Konten config.json berhasil dibaca");

                JsonDocument doc;
                if (deserializeJson(doc, content) == DeserializationError::Ok) {
                    Serial.println("[DEBUG] JSON parsing berhasil");
                    // Process MAC addresses
                    for (int i = 0; i < 2; i++) {
                        char key[10];
                        sprintf(key, "address%d", i);
                        Serial.printf("[DEBUG] Memproses key: %s\n", key);
                        
                        if (doc[key].is<JsonArray>()) {
                            JsonArray arr = doc[key];
                            Serial.printf("[DEBUG] Ukuran array untuk %s: %d\n", key, arr.size());
                            
                            if (arr.size() >= 2) {
                                const char* macStr = arr[0];
                                const char* devName = arr[1];
                                Serial.printf("[DEBUG] MAC: %s, Device: %s\n", macStr, devName);
                                
                                if (strlen(macStr) == 17) {
                                    uint8_t* targetMac = (i == 0) ? configData.macAddress : configData.macAddress1;
                                    char* targetName = (i == 0) ? configData.nameDevice1 : configData.nameDevice2;
                                    
                                    for (int j = 0; j < 6; j++) {
                                        targetMac[j] = strtoul(macStr + j*3, NULL, 16);
                                    }
                                    Serial.printf("[DEBUG] MAC address yang disimpan: %02X:%02X:%02X:%02X:%02X:%02X\n",
                                        targetMac[0], targetMac[1], targetMac[2], targetMac[3], targetMac[4], targetMac[5]);
                                    
                                    strncpy(targetName, devName, 11);
                                    Serial.printf("[DEBUG] Nama device yang disimpan: %s\n", targetName);
                                }
                            }
                        }
                    }
                } else {
                    Serial.println("[ERROR] Gagal parsing JSON config.json");
                }
            }
        }
    } else {
        Serial.println("[DEBUG] File config.json tidak ditemukan");
    }

    // Handle mode.json
    if (SPIFFS.exists("/mode.json")) {
        Serial.println("[DEBUG] File mode.json ditemukan");
        File file = SPIFFS.open("/mode.json", FILE_READ);
        if (file) {
            JsonDocument doc;
            if (deserializeJson(doc, file) == DeserializationError::Ok) {
                Serial.println("[DEBUG] JSON parsing mode.json berhasil");
                configData.dataInt = doc["volume"];
                Serial.printf("[DEBUG] Volume yang dibaca: %d\n", configData.dataInt);
                
                JsonArray modes = doc["mode"];
                Serial.printf("[DEBUG] Jumlah mode yang ditemukan: %d\n", modes.size());
                
                for (int i = 0; i < 9 && i < modes.size(); i++) {
                    configData.modeArray[i] = modes[i];
                    Serial.printf("[DEBUG] Mode[%d] = %d\n", i, configData.modeArray[i]);
                }
            } else {
                Serial.println("[ERROR] Gagal parsing JSON mode.json");
            }
            file.close();
        }
    } else {
        Serial.println("[DEBUG] File mode.json tidak ditemukan");
    }

    // Tidak perlu xSemaphoreGive
    Serial.println("[DEBUG] Inisialisasi storage selesai");
}

void storage::writeMacAddress(const uint8_t *mac, const char *device, int count) {
    // Tidak perlu ambil semaphore
    
    JsonDocument doc;
    File fileRead = SPIFFS.open("/config.json", FILE_READ);
    if (fileRead) {
        deserializeJson(doc, fileRead);
        fileRead.close();
    }

    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    bool slotFound = false;
    for (int i = 0; i < count; i++) {
        char key[10];
        sprintf(key, "address%d", i);
        
        if (/*!doc.containsKey(key) ||*/ !doc[key].is<JsonArray>() || doc[key].size() < 2) {
            // JsonArray arr = doc.createNestedArray(key);
            JsonArray arr = doc[key].to<JsonArray>();
            arr.add(macStr);
            arr.add(device);
            
            if (i == 0) {
                memcpy(configData.macAddress, mac, 6);
                strncpy(configData.nameDevice1, device, 11);
            } else {
                memcpy(configData.macAddress1, mac, 6);
                strncpy(configData.nameDevice2, device, 11);
            }
            slotFound = true;
            break;
        }
    }

    if (slotFound) {
        File fileWrite = SPIFFS.open("/config.json", FILE_WRITE);
        if (fileWrite) {
            serializeJson(doc, fileWrite);
            fileWrite.close();
        }
    }

    // Tidak perlu xSemaphoreGive
}

void storage::writeMode(const uint8_t *bufferMode, int count){
    // Tidak perlu ambil semaphore
    // Validasi parameter
    if (bufferMode == nullptr || count <= 0) {
        Serial.println("- bufferMode null atau count tidak valid");
        return;
    }
    JsonDocument doc;
    
    File fileRead = SPIFFS.open("/mode.json", FILE_READ);
    if (fileRead) {
        deserializeJson(doc, fileRead);
        fileRead.close();
    }

    bool slotFound = false;
    // Pastikan bufferMode di-cast dengan benar ke tipe int
    for (int i = 0; i < count; i++) {
        // Karena bufferMode bertipe uint8_t*, kita ambil 1 byte per mode
        doc["mode"][i] = bufferMode[i];
        configData.modeArray[i] = bufferMode[i];
        slotFound = true;
    }

    if (slotFound)
    {
        // Buka file untuk menulis mode
        File fileWrite = SPIFFS.open("/mode.json", FILE_WRITE);
        if (!fileWrite) {
            Serial.println("- gagal membuka file untuk menulis mode");
            return;
        }

        if (serializeJson(doc, fileWrite) == 0) {
            Serial.println("- gagal menulis JSON ke file mode.json");
        } else {
            Serial.println("Mode berhasil disimpan ke SPIFFS");
        }
        fileWrite.close();
    }
    // Tidak perlu xSemaphoreGive
}

void storage::writeMode(const int32_t* bufferMode, int count){
    // Tidak perlu ambil semaphore
    if (bufferMode == nullptr || count <= 0) {
        Serial.println("- bufferMode null atau count tidak valid");
        return;
    }

    JsonDocument doc;

    File fileRead = SPIFFS.open("/mode.json", FILE_READ);
    if (fileRead) {
        deserializeJson(doc, fileRead);
        fileRead.close();
    }

    bool slotFound = false;
    // Pastikan bufferMode di-cast dengan benar ke tipe int
    for (int i = 0; i < count; i++) {
        // Karena bufferMode bertipe uint8_t*, kita ambil 1 byte per mode
        doc["mode"][i] = bufferMode[i];
        configData.modeArray[i] = bufferMode[i];
        slotFound = true;
    }

    if (slotFound)
    {
        // Buka file untuk menulis mode
        File file = SPIFFS.open("/mode.json", FILE_WRITE);
        if (!file) {
            Serial.println("- gagal membuka file untuk menulis mode");
            return;
        }
        if (serializeJson(doc, file) == 0) {
            Serial.println("- gagal menulis JSON ke file mode.json");
        } else {
            Serial.println("Mode berhasil disimpan ke SPIFFS");
        }
        file.close();
    }
    // Tidak perlu xSemaphoreGive
}

// Fungsi untuk membaca mode dari SPIFFS dan mengkonversinya menjadi 8 variabel int
int* storage::readMode() {
    static int modeArray[8]; // static agar tetap valid setelah fungsi keluar

    File file = SPIFFS.open("/mode.json", FILE_READ);
    if (!file) {
        Serial.println("- gagal membuka file untuk membaca mode");
        // Jika gagal, set semua nilai ke 0
        for (int i = 0; i < 8; i++) {
            modeArray[i] = 0;
        }
        return modeArray;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    // if (error) {
    //     Serial.println("- gagal mendeserialize file mode.json");
    //     for (int i = 0; i < 8; i++) {
    //         modeArray[i] = 0;
    //     }
    //     return modeArray;
    // }

    // Membaca nilai mode dari dokumen JSON
    for (int i = 0; i < 8; i++) {
        if (doc["mode"][i].isNull()) {
            modeArray[i] = 0;
        } else {
            modeArray[i] = doc["mode"][i].as<int>();
        }
    }
    return modeArray;
}

void storage::saveVolume(int data){
    // Tidak perlu ambil semaphore
    // Buka file untuk menulis mode

    JsonDocument doc;

    File fileRead = SPIFFS.open("/mode.json", FILE_READ);
    if (fileRead) {
        deserializeJson(doc, fileRead);
        fileRead.close();
    }

    File file = SPIFFS.open("/mode.json", FILE_WRITE);
    if (!file) {
        Serial.println("- gagal membuka file untuk menulis mode");
        return;
    }

    // Karena bufferMode bertipe uint8_t*, kita ambil 1 byte per mode
    doc["volume"] = data;
    configData.dataInt = data;

    if (serializeJson(doc, file) == 0) {
        Serial.println("- gagal menulis JSON ke file mode.json");
    } else {
        Serial.println("Mode berhasil disimpan ke SPIFFS");
    }
    file.close();
    // Tidak perlu xSemaphoreGive
}

void storage::deleteAddress() {
    File file = SPIFFS.open("/config.json", FILE_WRITE);
    if (!file) {
        Serial.println("- failed to open file for writing");
        return;
    }
    
    JsonDocument doc;
    doc["address0"] = "";
    doc["address1"] = "";
    
    serializeJson(doc, file);
    file.close();
    memset(configData.macAddress, 0, 6);
    memset(configData.macAddress1, 0, 6);
    Serial.println("Addresses cleared successfully");
}

bool storage::hapusAlamat(const char *deviceName) {
    File fileRead = SPIFFS.open("/config.json", FILE_READ);
    if (!fileRead) {
        Serial.println("Gagal membuka config.json untuk dibaca.");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, fileRead);
    fileRead.close();

    if (error) {
        Serial.print("deserializeJson() gagal: ");
        Serial.println(error.c_str());
        return false;
    }

    JsonObject root = doc.as<JsonObject>();
    const char* keyToRemove = nullptr;

    for (JsonPair kv : root) {
        JsonArray arr = kv.value().as<JsonArray>();
        
        if (!arr.isNull() && arr.size() >= 2) {
            const char* currentDeviceName = arr[1];

            if (strcmp(currentDeviceName, deviceName) == 0) {
                
                keyToRemove = kv.key().c_str();
                Serial.printf("Perangkat '%s' ditemukan pada kunci '%s'. Akan dihapus.\n", deviceName, keyToRemove);
                break;
            }
        }
    }

    if (keyToRemove != nullptr) {

        if (strcmp(keyToRemove, "address0") == 0) {
            memset(configData.macAddress, 0, sizeof(configData.macAddress));
            memset(configData.nameDevice1,0,sizeof(configData.nameDevice1));
        } else if (strcmp(keyToRemove, "address1") == 0) {
            memset(configData.macAddress1, 0, sizeof(configData.macAddress1));
            memset(configData.nameDevice2,0,sizeof(configData.nameDevice2));
        }
        
        root.remove(keyToRemove);

        File fileWrite = SPIFFS.open("/config.json", FILE_WRITE);
        if (!fileWrite) {
            Serial.println("Gagal membuka config.json untuk ditulis.");
            return false;
        }

        if (serializeJson(doc, fileWrite) == 0) {
            Serial.println("Gagal menulis ke file.");
        } else {
            Serial.println("File config.json berhasil diperbarui.");
        }
        fileWrite.close();
    } else {
        Serial.printf("Perangkat dengan nama '%s' tidak ditemukan.\n", deviceName);
    }
    return true;
}