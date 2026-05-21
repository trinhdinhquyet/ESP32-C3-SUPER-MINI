package com.dwm.apiserver.config;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.scheduling.annotation.EnableAsync;
import org.springframework.scheduling.concurrent.ThreadPoolTaskExecutor;

import java.util.concurrent.Executor;

/**
 * Async Configuration for High-Throughput MQTT Processing
 * Xử lý bất đồng bộ cho throughput cao
 */
@Configuration
@EnableAsync
public class AsyncConfig {

    /**
     * MQTT Message Processing Thread Pool
     * Xử lý MQTT messages với thread pool riêng
     */
    @Bean(name = "mqttTaskExecutor")
    public Executor mqttTaskExecutor() {
        ThreadPoolTaskExecutor executor = new ThreadPoolTaskExecutor();

        // Core threads: số lượng thread chạy liên tục
        executor.setCorePoolSize(16);

        // Max threads: tăng lên 128 để xử lý 1000+ msg/s
        // Mỗi thread xử lý 1 message: 128 threads x 10ms/msg = 12,800 msg/s lý thuyết
        executor.setMaxPoolSize(128);

        // Queue capacity: tăng 5x (→ 10,000) để buffer spike traffic
        // Tại 1000 msg/s: queue đủ cho 10 giây spike trước khi sụp
        executor.setQueueCapacity(10000);

        // Thread name prefix (để debug dễ dàng)
        executor.setThreadNamePrefix("mqtt-async-");

        // Rejection policy: CallerRunsPolicy = backpressure (không drop message)
        executor.setRejectedExecutionHandler(new java.util.concurrent.ThreadPoolExecutor.CallerRunsPolicy());

        // Wait for tasks to complete on shutdown
        executor.setWaitForTasksToCompleteOnShutdown(true);
        executor.setAwaitTerminationSeconds(60);

        executor.initialize();
        return executor;
    }

    /**
     * Database Batch Write Thread Pool
     * Xử lý batch insert database với thread pool riêng
     */
    @Bean(name = "dbBatchExecutor")
    public Executor dbBatchExecutor() {
        ThreadPoolTaskExecutor executor = new ThreadPoolTaskExecutor();

        // DB batch processing không cần nhiều threads
        // Vì DB connection pool (30 conn) mới là bottleneck thực sự
        executor.setCorePoolSize(8);
        executor.setMaxPoolSize(16);

        // Tăng queue để bứuc được nhiều batch request hơn
        executor.setQueueCapacity(1000);
        executor.setThreadNamePrefix("db-batch-");
        executor.setRejectedExecutionHandler(new java.util.concurrent.ThreadPoolExecutor.CallerRunsPolicy());
        executor.setWaitForTasksToCompleteOnShutdown(true);
        executor.setAwaitTerminationSeconds(120);

        executor.initialize();
        return executor;
    }
}
