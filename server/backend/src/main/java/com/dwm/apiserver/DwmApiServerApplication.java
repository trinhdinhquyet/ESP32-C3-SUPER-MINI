package com.dwm.apiserver;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.builder.SpringApplicationBuilder;
import org.springframework.boot.web.servlet.support.SpringBootServletInitializer;
import org.springframework.scheduling.annotation.EnableAsync;
import org.springframework.scheduling.annotation.EnableScheduling;

/**
 * DWM API Server - Main Application
 * Equivalent to Python FastAPI app/main.py
 * 
 * Deploy options:
 * 1. Standalone: mvn spring-boot:run
 * 2. Tomcat WAR: mvn clean package -> deploy dwm-api-server.war to Tomcat
 */
@SpringBootApplication
@EnableAsync
@EnableScheduling
public class DwmApiServerApplication extends SpringBootServletInitializer {

    @Override
    protected SpringApplicationBuilder configure(SpringApplicationBuilder application) {
        // For WAR deployment on Tomcat
        return application.sources(DwmApiServerApplication.class);
    }

    public static void main(String[] args) {
        SpringApplication.run(DwmApiServerApplication.class, args);
        System.out.println("\n" +
            "╔═══════════════════════════════════════════════════════╗\n" +
            "║   ✅ DWM API Server Started Successfully!            ║\n" +
            "║   📡 MQTT Listener: Active                           ║\n" +
            "║   🗄️  Database: Connected                            ║\n" +
            "║   🔴 Redis: Connected                                 ║\n" +
            "║   🌐 API Base URL: http://localhost:8089              ║\n" +
            "║   📋 API Endpoints: /api/mqtt, /api/machines, etc.    ║\n" +
            "╚═══════════════════════════════════════════════════════╝\n"
        );
    }
}

