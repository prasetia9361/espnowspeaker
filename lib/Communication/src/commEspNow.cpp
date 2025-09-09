#include "commEspNow.h"

#ifdef RECEIVER
const char messaging[6] = "sound";
#endif

static commEspNow* instance = NULL;

// Callback untuk receiver
void receiverCallback(const uint8_t* macAddr, const uint8_t* data, int dataLen) {
    if (!instance || !macAddr || !data) {
        Serial.println("[ERROR] Null pointer in receiverCallback");
        return;
    }
    
    if (dataLen <= 0) {
        Serial.println("[ERROR] Invalid data length in receiverCallback");
        return;
    }
    
    if (strcmp((char*)data, "remot") == 0 || strcmp((char*)data, "displ") == 0) {
        Serial.println("[DEBUG] Menerima esp32remote/displ, menyimpan MAC address...");
        if (instance->memoryStorage) {
            instance->memoryStorage->writeMacAddress(macAddr,(char*)data, 2);
        }
    } else {
        if (!instance->memoryStorage) {
            Serial.println("[ERROR] memoryStorage is null");
            return;
        }
        
        int headerSize = instance->headerSize;
        if (dataLen >= sizeof(instance->messageData)) {
            memcpy(&instance->messageData, data, sizeof(instance->messageData));
        } else {
            Serial.println("[ERROR] Data length too small for messageData");
            return;
        }

        if (memcmp(macAddr, instance->memoryStorage->getMac(), 6) == 0 || 
            memcmp(macAddr, instance->memoryStorage->getMac1(), 6) == 0) {
            if (sizeof(instance->messageData.buffer) > headerSize && (sizeof(instance->messageData.buffer) <= maxEspNowPacketSize) && strlen(instance->messageData.data) == 0) {
                switch (instance->messageData.dataLen) {
                    case 0:
                        if (instance->audioBuffer) {
                            instance->audioBuffer->addBuffer(instance->messageData.buffer + headerSize, sizeof(instance->messageData.buffer) - headerSize);
                        }
                        break;
                    case 1:
                        for (int i = 0; i < 9; i++) {
                            Serial.print(instance->messageData.buffer[i]);
                            if (i < 8) Serial.print(", ");
                        }
                        Serial.println();
                        instance->memoryStorage->writeMode(instance->messageData.buffer, 9);
                        break;
                    default:
                        // Tambahkan penanganan jika diperlukan untuk dataLen selain 0 dan 1
                        break;
                }
                // Audio data
            } else {
                char jsonBuffer[256];
                memcpy(jsonBuffer, instance->messageData.data, sizeof(jsonBuffer) - 1);
                jsonBuffer[sizeof(jsonBuffer) - 1] = '\0';
                JsonDocument jsonDoc;
                DeserializationError error = deserializeJson(jsonDoc, jsonBuffer);
                if (!error) {
                    const char *header = jsonDoc["h"];
                    if (strcmp(header, "vol") == 0) {
                        Serial.print("[DEBUG] Volume speaker: ");
                        int vol = jsonDoc["d"];
                        instance->memoryStorage->saveVolume(vol);
                        Serial.println(vol);
                    } else if (strcmp(header, "test") == 0) {
                        Serial.print("[DEBUG] Button value: ");
                        instance->buttonValue = jsonDoc["d"];
                        Serial.println(instance->buttonValue);
                    }else if (strcmp(header, "remot") == 0)
                    {
                        Serial.print("[DEBUG] Button value: ");
                        int value = jsonDoc["d"];
                        int32_t* modeTones = instance->memoryStorage->readModeTones();
                        instance->buttonValue = modeTones[value];
                        Serial.println(instance->buttonValue);
                    }
                } else {
                    Serial.println("[DEBUG] Error parsing JSON");
                    Serial.print("[DEBUG] Error parsing JSON: ");
                    Serial.println(error.c_str());
                }
            }
        } else {
            Serial.println("[DEBUG] MAC address tidak cocok dengan yang tersimpan di storage");
        }
    }
}

commEspNow::~commEspNow() {
}

commEspNow::commEspNow(Buffer* audioBuffer, storage* memoryStorage, uint8_t wifiChannel) {
    Serial.println("[DEBUG] Inisialisasi commEspNow");
    instance = this;
    this->audioBuffer = audioBuffer;
    this->memoryStorage = memoryStorage;
    this->wifiChannel = wifiChannel;
    this->bufferSize = maxEspNowPacketSize;
    this->index = 0;
    this->headerSize = 0;
    this->lastData = 0;
    this->mac = nullptr;
}

bool commEspNow::begin() {
    Serial.println("[DEBUG] Memulai inisialisasi ESP-NOW");
    
    if (memoryStorage == nullptr) {
        Serial.println("[ERROR] memoryStorage tidak terinisialisasi");
        return false;
    }

    delay(100);
    
    WiFi.mode(WIFI_STA);
    delay(50);
    WiFi.disconnect();
    delay(100);
    
    Serial.print("[DEBUG] MAC Address: ");
    Serial.println(WiFi.macAddress());

    Serial.println("[DEBUG] Setup WiFi channel");
    esp_wifi_set_promiscuous(true);
    delay(10);
    esp_wifi_set_channel(wifiChannel, WIFI_SECOND_CHAN_NONE);
    delay(10);
    esp_wifi_set_promiscuous(false);
    delay(50);

    Serial.println("[DEBUG] Inisialisasi ESP-NOW");
    esp_err_t result = esp_now_init();
    if (result == ESP_OK) {
        delay(50); // Delay setelah ESP-NOW init
        
        #ifdef RECEIVER
            Serial.println("[DEBUG] ESPNow Init in Receiver Success");
            esp_now_register_recv_cb(receiverCallback);
        #endif
        
        delay(50); // Delay setelah register callback
    } else {
        Serial.printf("[ERROR] ESPNow Init failed: %s\n", esp_err_to_name(result));
        return false;
    }

    Serial.println("[DEBUG] Inisialisasi ESP-NOW berhasil");
    return true;
}

// Ubah return type menjadi bool agar kita tahu apakah penambahan peer berhasil atau tidak.
bool commEspNow::addPeer() {
    uint8_t* mac = nullptr; // Inisialisasi mac dengan nullptr untuk keamanan

    // Logika pengambilan MAC Anda (sudah benar)
    if (strcmp(memoryStorage->device1(), "sound") == 0) {
        mac = memoryStorage->getMac();
    } else if (strcmp(memoryStorage->device2(), "sound") == 0) {
        mac = memoryStorage->getMac1();
    }

    // Cek apakah alamat MAC sudah valid.
    // Jika tidak, langsung keluar dari fungsi tanpa delay.
    if (!mac || mac[0] == 0) {
        // Jangan delay di sini. Biarkan loop utama yang mengatur kapan harus mencoba lagi.
        return false; 
    }

    // Cek apakah peer sudah ada untuk menghindari penambahan berulang kali
    if (esp_now_is_peer_exist(mac)) {
        return true; // Anggap sudah berhasil jika peer sudah ada
    }

    // Jika peer belum ada, coba tambahkan
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;          // Gunakan channel 0, pastikan pengirim dan penerima di channel WiFi yang sama
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;  // Pastikan WiFi diinisialisasi dalam mode STA

    esp_err_t result = esp_now_add_peer(&peerInfo);
    
    if (result == ESP_OK) {
        Serial.printf("Peer added successfully with MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return true; // Berhasil menambahkan peer
    } else {
        Serial.printf("[ERROR] Failed to add peer: %s\n", esp_err_to_name(result));
        return false; // Gagal menambahkan peer
    }
}

bool commEspNow::addPeer(const uint8_t* _mac){
    mac = _mac;
    if (mac == nullptr || mac[0] == 0) {
        Serial.println("MAC address null, tidak bisa menambah peer.");
        return false;
    }

    if (!esp_now_is_peer_exist(mac)) {
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, mac, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;
        peerInfo.ifidx = WIFI_IF_STA;
        esp_err_t res = esp_now_add_peer(&peerInfo);
        if (res == ESP_OK) {
            Serial.println("Peer berhasil ditambahkan.");
            return true;
        } else {
            Serial.print("Gagal menambah peer: ");
            Serial.println(esp_err_to_name(res));
            return false;
        }
    }
    // Peer sudah ada
    return true;
}

void commEspNow::sendData() {
    esp_err_t result = esp_now_send(mac, (uint8_t*)&messageData, sizeof(messageData));
    if (result != ESP_OK) {
        Serial.printf("Send Error: %s\n", esp_err_to_name(result));
        
        // Handle error spesifik
        if (result == ESP_ERR_ESPNOW_NO_MEM) {
            vTaskDelay(pdMS_TO_TICKS(10)); // Beri waktu pemrosesan
        }
    }
}

void commEspNow::addSample(int16_t sample) {
    
    // if (mac == nullptr || mac[0] == 0) {
    //     Serial.println("MAC address null, tidak bisa menambah peer.");
    //     return;
    // }
    
    // if (!esp_now_is_peer_exist(mac)) {
    //     Serial.println("Peer not registered, adding...");
    //     addPeer(mac);
    // }
    // messageData.buffer[index + headerSize] = (sample + 2048) * 256 / 4096;
    // messageData.buffer[index + headerSize] = (sample + 512) * 256 / 1024;
    messageData.buffer[index + headerSize] = (sample + 32768) >> 8;
    index++;

    if ((index + headerSize) == bufferSize) {
        messageData.dataLen = 0;
        sendData();
        index = 0;
    }
}

void commEspNow::sendDataInt(int data, const char *header) {
    bool isVolHeader = (strcmp(header, "vol") == 0);
    if (!isVolHeader && data == lastData) {
        return;
    }

    Serial.print("[DEBUG] Mengirim data integer. Header: ");
    Serial.print(header);
    Serial.print(", Data: ");
    Serial.println(data);

    JsonDocument doc;
    doc["h"] = header;
    doc["d"] = data;

    serializeJson(doc, messageData.data);
    messageData.dataLen = 1;

    sendData();
    memset(messageData.data, 0, sizeof(messageData.data));
    if (!isVolHeader) {
        lastData = data;
        Serial.print("[DEBUG] lastData diupdate menjadi: ");
        Serial.println(lastData);
    }
}

void commEspNow::sendModeSiren(const uint8_t *modelBuffer){
    memcpy(messageData.buffer, modelBuffer, sizeof(messageData.buffer));
    memset(messageData.data, 0, sizeof(messageData.data));
    messageData.dataLen = 1;
    sendData();
}

void commEspNow::flush() {
    if (index > 0) {
        sendData();
        index = 0;
    }
}

void commEspNow::statusBinding() {
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;

    // Set broadcast address
    for (int i = 0; i < 6; i++) {
        peerInfo.peer_addr[i] = 0xFF;
    }

    if (esp_now_add_peer(&peerInfo) == ESP_OK) {
        Serial.println(messaging);
        esp_now_send(peerInfo.peer_addr, (uint8_t*)messaging, 12);
        esp_now_del_peer(peerInfo.peer_addr);
    }
}

bool commEspNow::binding() {
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;

    // Set broadcast address
    for (int i = 0; i < 6; i++) {
        peerInfo.peer_addr[i] = 0xFF;
    }

    // uint8_t custom_addr[6] = {0x34, 0x85, 0x18, 0x8F, 0xBD, 0x04};
    // memcpy(peerInfo.peer_addr, custom_addr, 6);

    if (esp_now_add_peer(&peerInfo) == ESP_OK) {
        Serial.println(messaging);
        esp_err_t result = esp_now_send(peerInfo.peer_addr, (uint8_t*)messaging, 12);
        if (result != ESP_OK) {
            Serial.printf("Send Error: %s\n", esp_err_to_name(result));
        }else
        {
            Serial.printf("Send Success");
        }
        esp_now_del_peer(peerInfo.peer_addr);
        
        return true;
    }
    return false;
}

int commEspNow::setHeader(const int headerSize, const uint8_t* header) {
    if ((headerSize < bufferSize) && (header)) {
        this->headerSize = headerSize;
        memcpy(messageData.buffer, header, headerSize);
        return 0;
    } else {
        return -1;
    }
}