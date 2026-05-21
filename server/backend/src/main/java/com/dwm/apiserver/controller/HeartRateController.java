package com.dwm.apiserver.controller;

import com.dwm.apiserver.model.HeartRateRecord;
import com.dwm.apiserver.repository.HeartRateRecordRepository;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.NoArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.servlet.mvc.method.annotation.SseEmitter;

import java.io.IOException;
import java.time.LocalDateTime;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.stream.Collectors;

@Slf4j
@RestController
@RequestMapping("/api/esp32")
@CrossOrigin(origins = "*")
public class HeartRateController {

    private final HeartRateRecordRepository repository;
    
    // Thread-safe list to hold active SSE connections
    private final List<SseEmitter> emitters = new CopyOnWriteArrayList<>();

    // Track active devices: deviceId -> last seen timestamp
    private final Map<String, LocalDateTime> activeDevices = new ConcurrentHashMap<>();
    
    // Consider a device offline if not seen for this many seconds
    private static final int DEVICE_TIMEOUT_SECONDS = 30;

    public HeartRateController(HeartRateRecordRepository repository) {
        this.repository = repository;
    }

    // DTO for incoming ESP32 data (includes PPG raw samples for real-time charting)
    @Data
    @NoArgsConstructor
    @AllArgsConstructor
    @Builder
    public static class HeartRatePayload {
        private String deviceId;
        private Integer heartRate;
        private Integer spo2;
        private List<PpgSample> samples;
    }

    @Data
    @NoArgsConstructor
    @AllArgsConstructor
    public static class PpgSample {
        private Long ir;
        private Long red;
    }

    @Data
    @NoArgsConstructor
    @AllArgsConstructor
    public static class DeviceInfo {
        private String deviceId;
        private boolean online;
        private LocalDateTime lastSeen;
    }

    /**
     * Common processing logic for telemetry data (called by both REST and MQTT)
     */
    public void processHeartRateData(HeartRatePayload payload) {
        log.info("Processing data from {}: HeartRate: {} BPM | SpO2: {}% | Raw Samples: {}", 
                payload.getDeviceId(), payload.getHeartRate(), payload.getSpo2(), 
                payload.getSamples() != null ? payload.getSamples().size() : 0);

        // Track device as active
        String deviceId = payload.getDeviceId() != null ? payload.getDeviceId() : "ESP32-C3";
        activeDevices.put(deviceId, LocalDateTime.now());

        // 1. Save summary metrics to database (only if heart rate is non-zero, indicating finger is present)
        if (payload.getHeartRate() != null && payload.getHeartRate() > 0) {
            HeartRateRecord record = HeartRateRecord.builder()
                    .deviceId(deviceId)
                    .heartRate(payload.getHeartRate())
                    .spo2(payload.getSpo2())
                    .timestamp(LocalDateTime.now())
                    .build();
            try {
                repository.save(record);
            } catch (Exception e) {
                log.error("Failed to save heart rate record to DB: {}", e.getMessage());
            }
        }

        // 2. Broadcast raw samples + metrics to all connected SSE clients (Web frontends)
        broadcast(payload);
    }

    /**
     * Endpoint for HTTP POST uploads (useful for backward compatibility or testing)
     */
    @PostMapping("/data")
    public String receiveData(@RequestBody HeartRatePayload payload) {
        processHeartRateData(payload);
        return "OK";
    }

    /**
     * SSE Stream Endpoint for Web Frontends
     */
    @GetMapping(value = "/stream", produces = MediaType.TEXT_EVENT_STREAM_VALUE)
    public SseEmitter streamData() {
        // Create an emitter with a 5-minute timeout (300,000 ms)
        SseEmitter emitter = new SseEmitter(300_000L);
        emitters.add(emitter);

        emitter.onCompletion(() -> emitters.remove(emitter));
        emitter.onTimeout(() -> emitters.remove(emitter));
        emitter.onError((ex) -> emitters.remove(emitter));

        // Send a connection confirmation event
        try {
            emitter.send(SseEmitter.event()
                    .name("init")
                    .data("Connected to ESP32 Heart Rate Stream"));
            log.info("New web client connected to SSE heart rate stream. Active clients: {}", emitters.size());
        } catch (IOException e) {
            emitters.remove(emitter);
        }

        return emitter;
    }

    /**
     * Endpoint to fetch the list of all devices (both active and historical)
     */
    @GetMapping("/devices")
    public List<DeviceInfo> getDevices() {
        // Query database for all devices and their last record timestamp
        List<Object[]> dbDevices = repository.findDeviceLastSeenTimestamps();
        
        Map<String, LocalDateTime> deviceLastSeenMap = new HashMap<>();
        for (Object[] row : dbDevices) {
            String devId = (String) row[0];
            LocalDateTime ts = (LocalDateTime) row[1];
            if (devId != null) {
                deviceLastSeenMap.put(devId, ts);
            }
        }
        
        // Incorporate any active devices from activeDevices map (in-memory)
        LocalDateTime cutoff = LocalDateTime.now().minusSeconds(DEVICE_TIMEOUT_SECONDS);
        for (Map.Entry<String, LocalDateTime> entry : activeDevices.entrySet()) {
            String devId = entry.getKey();
            LocalDateTime ts = entry.getValue();
            if (ts.isAfter(cutoff)) {
                // Active device has more recent activity than stored in DB
                LocalDateTime existing = deviceLastSeenMap.get(devId);
                if (existing == null || ts.isAfter(existing)) {
                    deviceLastSeenMap.put(devId, ts);
                }
            }
        }
        
        return deviceLastSeenMap.entrySet().stream()
                .map(entry -> {
                    String devId = entry.getKey();
                    LocalDateTime ts = entry.getValue();
                    boolean online = activeDevices.containsKey(devId) && activeDevices.get(devId).isAfter(cutoff);
                    return new DeviceInfo(devId, online, ts);
                })
                .sorted(Comparator.comparing(DeviceInfo::getDeviceId))
                .collect(Collectors.toList());
    }

    /**
     * Endpoint to fetch the last 100 historical readings for trend initialization.
     * Optionally filter by deviceId query param.
     */
    @GetMapping("/history")
    public List<HeartRateRecord> getHistory(@RequestParam(required = false) String deviceId) {
        List<HeartRateRecord> records;
        if (deviceId != null && !deviceId.isBlank()) {
            records = repository.findTop100ByDeviceIdOrderByTimestampDesc(deviceId);
        } else {
            records = repository.findTop100ByOrderByTimestampDesc();
        }
        // Reverse to display chronologically (left to right in charts)
        Collections.reverse(records);
        return records;
    }

    /**
     * Helper method to broadcast telemetry payload to all active SSE subscribers
     */
    private void broadcast(HeartRatePayload payload) {
        List<SseEmitter> deadEmitters = new ArrayList<>();
        
        for (SseEmitter emitter : emitters) {
            try {
                emitter.send(SseEmitter.event()
                        .name("pulse-data")
                        .data(payload));
            } catch (Exception e) {
                deadEmitters.add(emitter);
            }
        }
        
        if (!deadEmitters.isEmpty()) {
            emitters.removeAll(deadEmitters);
            log.info("Removed {} dead SSE emitters. Remaining active clients: {}", deadEmitters.size(), emitters.size());
        }
    }
}
