package com.dwm.apiserver.controller;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.HashMap;
import java.util.Map;

/**
 * Root Controller
 * Equivalent to Python @app.get("/")
 * 
 * FIX #11: Removed hardcoded IP, uses server.port from config
 */
@RestController
@RequestMapping("/")
public class RootController {

    @Value("${spring.application.name:DWM API Server}")
    private String appName;

    @Value("${server.port:8089}")
    private String serverPort;

    @GetMapping
    public ResponseEntity<Map<String, String>> root() {
        Map<String, String> response = new HashMap<>();
        response.put("message", appName + " 运行中");
        response.put("status", "running");
        response.put("api_endpoints", "http://localhost:" + serverPort + "/api");
        return ResponseEntity.ok(response);
    }

    /**
     * Handle favicon.ico requests to avoid 404 errors
     */
    @GetMapping("/logotron.ico")
    public ResponseEntity<Void> favicon() {
        return ResponseEntity.status(HttpStatus.NO_CONTENT).build();
    }
}
