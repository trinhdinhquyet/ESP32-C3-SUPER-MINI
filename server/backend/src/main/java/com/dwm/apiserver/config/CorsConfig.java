package com.dwm.apiserver.config;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.web.cors.CorsConfiguration;
import org.springframework.web.cors.UrlBasedCorsConfigurationSource;
import org.springframework.web.filter.CorsFilter;

import java.util.Arrays;

/**
 * CORS Configuration
 * Equivalent to Python FastAPI CORSMiddleware in app/main.py
 * 
 * Cấu hình cho phép TẤT CẢ origins truy cập API (không cần thêm từng địa chỉ)
 * Phù hợp cho môi trường mạng nội bộ
 */
@Configuration
public class CorsConfig {

    @Bean
    public CorsFilter corsFilter() {
        UrlBasedCorsConfigurationSource source = new UrlBasedCorsConfigurationSource();
        CorsConfiguration config = new CorsConfiguration();

        // Cho phép TẤT CẢ origins - không cần cấu hình trong application.properties
        // Phù hợp cho mạng nội bộ, không cần thêm từng địa chỉ IP/port
        config.setAllowedOriginPatterns(Arrays.asList("*"));
        config.setAllowCredentials(false);

        // Allowed methods
        config.setAllowedMethods(Arrays.asList("GET", "POST", "PUT", "DELETE", "OPTIONS", "PATCH"));

        // Allow all headers
        config.addAllowedHeader("*");

        // Exposed headers
        config.addExposedHeader("*");

        // Max age for preflight (1 hour)
        config.setMaxAge(3600L);

        // Apply CORS configuration to all paths
        source.registerCorsConfiguration("/**", config);

        return new CorsFilter(source);
    }
}

