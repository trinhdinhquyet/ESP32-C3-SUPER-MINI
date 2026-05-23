#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoOTA.h>

// ============================================
// WiFi & EMQX Server Configuration
// ============================================
const char* ssid = "HLIG KMP";
const char* password = ""; // Leave empty if your WiFi doesn't have a password
static const char *mqttServer = "10.32.10.205";
static const int mqttPort = 1883;
static const char *mqttUser = "andon";
static const char *mqttPassword = "adminwlv";
static const char *mqttTopic = "heartrate";

String mp3DeviceName = ""; // Will be initialized with MAC address at runtime
const char* mqttOtaTopic = "updateFirmware";
#ifndef OTA_AUTH_PASSWORD
#define OTA_AUTH_PASSWORD "c3ota"
#endif

MAX30105 particleSensor;

#define SDA_PIN 8
#define SCL_PIN 9

#ifdef BUFFER_SIZE
#undef BUFFER_SIZE
#endif
#define BUFFER_SIZE 100

uint32_t irBuffer[BUFFER_SIZE];
uint32_t redBuffer[BUFFER_SIZE];

int32_t spo2;
int8_t validSPO2;

int32_t heartRate;
int8_t validHeartRate;

// Timer and interval configurations
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL_MS = 2000; // Send interval (e.g. 1000 = 1s, 2000 = 2s)

WiFiClient espClient;
PubSubClient client(espClient);

// ============================================
// OTA configurations & functions
// ============================================
static bool isHttpUrl(const char* s) {
    if (!s || !s[0]) return false;
    return (strncmp(s, "http://", 7) == 0) || (strncmp(s, "https://", 8) == 0);
}

static void runHttpOtaFromUrl(const char* url) {
    if (!isHttpUrl(url)) {
        return;
    }

    Serial.printf("[OTA] Starting HTTP update from: %s\n", url);
    client.disconnect();
    delay(200);

    httpUpdate.onStart([]() {
        Serial.println("[OTA] HTTP update started");
    });
    httpUpdate.onEnd([]() {
        Serial.println("[OTA] HTTP update finished successfully");
    });
    httpUpdate.onProgress([](int current, int total) {
        Serial.printf("[OTA] HTTP update progress: %d/%d\n", current, total);
    });
    httpUpdate.onError([](int err) {
        Serial.printf("[OTA] HTTP update error code: %d\n", err);
    });

    t_httpUpdate_return ret = HTTP_UPDATE_FAILED;
    if (strncmp(url, "https://", 8) == 0) {
        WiFiClientSecure sec;
        sec.setInsecure();
        ret = httpUpdate.update(sec, url);
    } else {
        WiFiClient updateClient;
        ret = httpUpdate.update(updateClient, url);
    }

    if (ret == HTTP_UPDATE_FAILED) {
        Serial.printf("[OTA] HTTP update failed: %s\n", httpUpdate.getLastErrorString().c_str());
    }
}

static void handleOtaMqttPayload(const char* message) {
    if (!message || !message[0]) return;

    const char* url = message;
    char buf[96];
    const char* sep = strchr(message, '|');
    if (sep) {
        size_t nameLen = static_cast<size_t>(sep - message);
        if (nameLen >= sizeof(buf)) return;
        memcpy(buf, message, nameLen);
        buf[nameLen] = '\0';
        // Trim trailing whitespace
        while (nameLen > 0 && (buf[nameLen - 1] == ' ' || buf[nameLen - 1] == '\t')) {
            buf[--nameLen] = '\0';
        }
        if (strcmp(buf, mp3DeviceName.c_str()) != 0) {
            Serial.printf("[OTA] Ignored. Target device: %s, My name: %s\n", buf, mp3DeviceName.c_str());
            return;
        }
        url = sep + 1;
        while (*url == ' ' || *url == '\t') url++;
    }

    if (!isHttpUrl(url)) return;
    runHttpOtaFromUrl(url);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (!topic || strcmp(topic, mqttOtaTopic) != 0) {
        return;
    }
    if (length == 0 || length > 512) {
        return;
    }
    char msg[513];
    memcpy(msg, payload, length);
    msg[length] = '\0';
    Serial.printf("[MQTT] OTA Command received: %s\n", msg);
    handleOtaMqttPayload(msg);
}

void setupArduinoOTA() {
    ArduinoOTA.setHostname(("esp32-c3-pulse-" + mp3DeviceName).c_str());
    static const char kOtaPwd[] = OTA_AUTH_PASSWORD;
    if (kOtaPwd[0] != '\0') {
        ArduinoOTA.setPassword(kOtaPwd);
    }

    ArduinoOTA.onStart([]() {
        Serial.println("[ArduinoOTA] Start");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\n[ArduinoOTA] End");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[ArduinoOTA] Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[ArduinoOTA] Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    if (WiFi.status() == WL_CONNECTED) {
        ArduinoOTA.begin();
        Serial.println("[ArduinoOTA] Ready");
    }
}

void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    Serial.printf("\n[WiFi] Connecting to SSID: %s ...\n", ssid);
    WiFi.mode(WIFI_STA);
    
    // Get MAC Address to set device name
    if (mp3DeviceName == "") {
        uint8_t mac[6];
        WiFi.macAddress(mac);
        char macBuf[13];
        snprintf(macBuf, sizeof(macBuf), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        mp3DeviceName = String(macBuf);
        Serial.printf("[Device] MAC Address (Device Name): %s\n", mp3DeviceName.c_str());
    }
    
    // Set lower WiFi TX Power to reduce heat (default is WIFI_POWER_19_5Dbm, which draws ~300mA)
    // 8.5dBm is enough for indoor communication and consumes much less power
    WiFi.setTxPower(WIFI_POWER_8_5dBm); 

    while (WiFi.status() != WL_CONNECTED) {
        if (password != NULL && strlen(password) > 0) {
            WiFi.begin(ssid, password);
        } else {
            WiFi.begin(ssid);
        }
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("\n[WiFi] Connection failed. Retrying in 2 seconds...");
            WiFi.disconnect();
            delay(2000);
        }
    }
    Serial.println("\n[WiFi] Connected successfully!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());
}

void reconnectMqtt() {
    int retries = 0;
    Serial.printf("[MQTT] Connecting to broker: %s:%d ...\n", mqttServer, mqttPort);

    while (!client.connected() && retries < 3) {
        String clientId = "ESP32-C5555-" + mp3DeviceName;
        Serial.printf("[MQTT] Attempting connection using ClientId: %s (try %d/3)...\n", clientId.c_str(), retries + 1);
        if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
            Serial.println("[MQTT] Connected successfully to EMQX!");
            client.subscribe(mqttOtaTopic);
            Serial.printf("[MQTT] Subscribed to OTA topic: %s\n", mqttOtaTopic);
        } else {
            Serial.printf("[MQTT] Connection failed, state rc=%d. Retrying in 2 seconds...\n", client.state());
            delay(2000);
            retries++;
        }
    }
}

void setup()
{
    Serial.begin(115200);
    
    // Wait for Serial Monitor to connect (max 2.5 seconds)
    unsigned long startSerial = millis();
    while (!Serial && (millis() - startSerial < 2500)) {
        delay(10);
    }
    Serial.println("\n--- ESP32-C3 Booting (Optimized for Power & Heat) ---");

    // Auto WiFi connection
    connectWiFi();

    client.setServer(mqttServer, mqttPort);
    client.setCallback(mqttCallback);
    setupArduinoOTA();

    Wire.begin(SDA_PIN, SCL_PIN);

    if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
    {
        while (1) {
            delay(5000);
        }
    }

    byte ledBrightness = 15;
    byte sampleAverage = 4;
    byte ledMode = 2;
    int sampleRate = 100;
    int pulseWidth = 411;
    int adcRange = 4096;

    particleSensor.setup(
        ledBrightness,
        sampleAverage,
        ledMode,
        sampleRate,
        pulseWidth,
        adcRange);

    particleSensor.setPulseAmplitudeRed(0x3F);
    particleSensor.setPulseAmplitudeIR(0x1F);
}

void loop()
{
    if (WiFi.status() == WL_CONNECTED) {
        ArduinoOTA.handle();
    }

    // Auto-reconnect WiFi if disconnected
    if (WiFi.status() != WL_CONNECTED) {
        static unsigned long lastWifiRetry = 0;
        if (millis() - lastWifiRetry > 10000) {
            Serial.println("[WiFi] Connection lost! Initiating reconnection...");
            connectWiFi();
            lastWifiRetry = millis();
        }
    }

    // Auto-reconnect MQTT if WiFi is connected and MQTT is disconnected
    if (WiFi.status() == WL_CONNECTED && !client.connected()) {
        static unsigned long lastMqttRetry = 0;
        if (millis() - lastMqttRetry > 10000) {
            Serial.println("[MQTT] Connection lost! Initiating reconnection...");
            reconnectMqtt();
            lastMqttRetry = millis();
        }
    }

    if (client.connected()) {
        client.loop();
    }

    // Read new sample from sensor (non-blocking)
    particleSensor.check();
    if (particleSensor.available())
    {
        long currentRed = particleSensor.getRed();
        long currentIR = particleSensor.getIR();
        particleSensor.nextSample();

        static bool fingerWasPresent = false;
        static int lastValidBPM = 0;
        static int lastValidSPO2 = 0;

        // Check if finger is present (IR/Red values drop when no finger is placed)
        if (currentIR < 60000 || currentRed < 60000)
        {
            fingerWasPresent = false;

            // If finger is removed, send update to server every SEND_INTERVAL_MS (keep-alive + status)
            if (millis() - lastSendTime > SEND_INTERVAL_MS) {
                lastSendTime = millis();
                lastValidBPM = 0;
                lastValidSPO2 = 0;
                
                if (client.connected()) {
                    String idlePayload = "{\"deviceId\":\"" + mp3DeviceName + "\",\"heartRate\":0,\"spo2\":0,\"samples\":[]}";
                    client.publish(mqttTopic, idlePayload.c_str());
                }
            }
        }
        else
        {
            // Transition: finger just detected
            if (fingerWasPresent == false)
            {
                for (byte i = 0; i < BUFFER_SIZE; i++)
                {
                    irBuffer[i] = currentIR;
                    redBuffer[i] = currentRed;
                }
                fingerWasPresent = true;
            }

            // Slide algorithmic buffers
            for (byte i = 1; i < BUFFER_SIZE; i++)
            {
                redBuffer[i - 1] = redBuffer[i];
                irBuffer[i - 1] = irBuffer[i];
            }
            redBuffer[BUFFER_SIZE - 1] = currentRed;
            irBuffer[BUFFER_SIZE - 1] = currentIR;

            // Periodically run algorithm (every 10 samples ~400ms)
            static int calcCounter = 0;
            calcCounter++;

            if (calcCounter >= 10)
            {
                calcCounter = 0;

                maxim_heart_rate_and_oxygen_saturation(
                    irBuffer,
                    BUFFER_SIZE,
                    redBuffer,
                    &spo2,
                    &validSPO2,
                    &heartRate,
                    &validHeartRate);

                if (validHeartRate && heartRate > 45 && heartRate < 130)
                {
                    lastValidBPM = heartRate;
                }
                if (validSPO2 && spo2 > 70 && spo2 <= 100)
                {
                    lastValidSPO2 = spo2;
                }
            }

            // Send active telemetry periodically (every SEND_INTERVAL_MS)
            if (millis() - lastSendTime > SEND_INTERVAL_MS) {
                lastSendTime = millis();

                if (client.connected()) {
                    String payload = "{\"deviceId\":\"" + mp3DeviceName + "\",\"heartRate\":" + String(lastValidBPM) + 
                                     ",\"spo2\":" + String(lastValidSPO2) + ",\"samples\":[]}";
                    
                    client.publish(mqttTopic, payload.c_str());
                }
            }
        }
    }

    // Yield CPU time to FreeRTOS (helps reduce power consumption & heat dramatically)
    delay(1); 
}