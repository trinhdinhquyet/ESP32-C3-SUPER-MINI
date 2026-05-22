#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

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
#define MP3_DEVICE_NAME "A1"

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

void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    Serial.printf("\n[WiFi] Connecting to SSID: %s ...\n", ssid);
    WiFi.mode(WIFI_STA);
    
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
        String clientId = "ESP32-C3-Pulse-" + String(random(0xffff), HEX);
        Serial.printf("[MQTT] Attempting connection using ClientId: %s (try %d/3)...\n", clientId.c_str(), retries + 1);
        if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
            Serial.println("[MQTT] Connected successfully to EMQX!");
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
                    String idlePayload = "{\"deviceId\":\"" + String(MP3_DEVICE_NAME) + "\",\"heartRate\":0,\"spo2\":0,\"samples\":[]}";
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
                    String payload = "{\"deviceId\":\"" + String(MP3_DEVICE_NAME) + "\",\"heartRate\":" + String(lastValidBPM) + 
                                     ",\"spo2\":" + String(lastValidSPO2) + ",\"samples\":[]}";
                    
                    client.publish(mqttTopic, payload.c_str());
                }
            }
        }
    }

    // Yield CPU time to FreeRTOS (helps reduce power consumption & heat dramatically)
    delay(1); 
}