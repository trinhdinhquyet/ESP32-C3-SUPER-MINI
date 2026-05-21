package com.dwm.apiserver.controller;

import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.time.Instant;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import java.util.HashMap;
import java.util.Map;

/**
 * Server Time Controller
 * Returns current server time
 */
@RestController
@RequestMapping("/api")
public class ServerTimeController {

    /**
     * Get current server time
     * Equivalent to: GET /api/server-time
     */
    @GetMapping("/server-time")
    public ResponseEntity<Map<String, String>> getServerTime() {
        // Get current server time
        Instant now = Instant.now();
        ZonedDateTime serverTime = now.atZone(ZoneId.systemDefault());

        // Format timestamps
        DateTimeFormatter simpleFormatter = DateTimeFormatter.ofPattern("yyyy-MM-dd'T'HH:mm:ss.SSS");

        String isoTime = serverTime.format(DateTimeFormatter.ISO_INSTANT);
        String simpleTime = serverTime.format(simpleFormatter);

        // Get timezone info
        String timezone = serverTime.getZone().getId();
        int offsetHours = serverTime.getOffset().getTotalSeconds() / 3600;

        // Build response
        Map<String, String> response = new HashMap<>();
        response.put("timestamp", isoTime);
        response.put("time", simpleTime);
        response.put("datetime", isoTime);
        response.put("server_timezone", timezone);
        response.put("utc_offset", String.valueOf(offsetHours));
        response.put("server_time_with_tz", serverTime.format(DateTimeFormatter.ISO_OFFSET_DATE_TIME));

        return ResponseEntity.ok(response);
    }
}
