package com.dwm.apiserver.repository;

import com.dwm.apiserver.model.HeartRateRecord;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.stereotype.Repository;

import java.util.List;

@Repository
public interface HeartRateRecordRepository extends JpaRepository<HeartRateRecord, Long> {
    
    // Get the most recent records, ordered by timestamp descending
    List<HeartRateRecord> findTop100ByOrderByTimestampDesc();
    
    // Find records by device id
    List<HeartRateRecord> findByDeviceIdOrderByTimestampDesc(String deviceId);

    // Find top 100 records by device id
    List<HeartRateRecord> findTop100ByDeviceIdOrderByTimestampDesc(String deviceId);

    // Get all device IDs and their latest record timestamps
    @Query("SELECT r.deviceId, MAX(r.timestamp) FROM HeartRateRecord r GROUP BY r.deviceId")
    List<Object[]> findDeviceLastSeenTimestamps();
}
