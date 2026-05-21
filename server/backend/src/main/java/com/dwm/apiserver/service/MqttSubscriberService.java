package com.dwm.apiserver.service;

import com.dwm.apiserver.controller.HeartRateController;
import com.dwm.apiserver.controller.HeartRateController.HeartRatePayload;
import com.fasterxml.jackson.databind.ObjectMapper;
import jakarta.annotation.PostConstruct;
import jakarta.annotation.PreDestroy;
import lombok.extern.slf4j.Slf4j;
import org.eclipse.paho.client.mqttv3.*;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

@Slf4j
@Service
public class MqttSubscriberService {

    private final HeartRateController heartRateController;
    private final ObjectMapper objectMapper;

    @Value("${mqtt.broker.url}")
    private String brokerUrl;

    @Value("${mqtt.client.id}")
    private String clientId;

    @Value("${mqtt.username}")
    private String username;

    @Value("${mqtt.password}")
    private String password;

    @Value("${mqtt.topic}")
    private String topic;

    private MqttClient mqttClient;

    public MqttSubscriberService(HeartRateController heartRateController, ObjectMapper objectMapper) {
        this.heartRateController = heartRateController;
        this.objectMapper = objectMapper;
    }

    @PostConstruct
    public void start() {
        connectAndSubscribe();
    }

    @PreDestroy
    public void stop() {
        disconnect();
    }

    private void connectAndSubscribe() {
        try {
            log.info("Connecting to EMQX Broker at {}...", brokerUrl);
            mqttClient = new MqttClient(brokerUrl, clientId, new MemoryPersistence());
            
            MqttConnectOptions options = new MqttConnectOptions();
            options.setUserName(username);
            options.setPassword(password.toCharArray());
            options.setCleanSession(true);
            options.setAutomaticReconnect(true);
            options.setConnectionTimeout(10);
            options.setKeepAliveInterval(30);

            mqttClient.setCallback(new MqttCallbackExtended() {
                @Override
                public void connectComplete(boolean reconnect, String serverURI) {
                    log.info("✅ Connected to EMQX Broker: reconnect={}, URI={}", reconnect, serverURI);
                    subscribeToTopic();
                }

                @Override
                public void connectionLost(Throwable cause) {
                    log.warn("⚠️ EMQX MQTT connection lost: {}", cause != null ? cause.getMessage() : "unknown reason");
                }

                @Override
                public void messageArrived(String topic, MqttMessage message) {
                    String payloadString = new String(message.getPayload());
                    log.debug("Received MQTT message on topic: {} (length: {})", topic, payloadString.length());
                    
                    try {
                        // Parse JSON payload into HeartRatePayload DTO
                        HeartRatePayload payload = objectMapper.readValue(payloadString, HeartRatePayload.class);
                        // Forward to controller to process database saving and SSE broadcast
                        heartRateController.processHeartRateData(payload);
                    } catch (Exception e) {
                        log.error("❌ Failed to parse incoming MQTT JSON message: {}", e.getMessage());
                    }
                }

                @Override
                public void deliveryComplete(IMqttDeliveryToken token) {
                    // Not publishing, so no-op
                }
            });

            mqttClient.connect(options);

        } catch (MqttException e) {
            log.error("❌ Failed to connect to EMQX MQTT Broker: {}", e.getMessage());
        }
    }

    private void subscribeToTopic() {
        if (mqttClient == null || !mqttClient.isConnected()) {
            return;
        }
        try {
            mqttClient.subscribe(topic, 1);
            log.info("✅ Subscribed to MQTT topic: {}", topic);
        } catch (MqttException e) {
            log.error("❌ Failed to subscribe to topic {}: {}", topic, e.getMessage());
        }
    }

    private void disconnect() {
        if (mqttClient != null && mqttClient.isConnected()) {
            try {
                mqttClient.disconnect();
                mqttClient.close();
                log.info("Disconnected from EMQX broker successfully");
            } catch (MqttException e) {
                log.error("Failed to disconnect from EMQX broker: {}", e.getMessage());
            }
        }
    }
}
