# DWM API Server - Spring Boot Version

![Spring Boot](https://img.shields.io/badge/Spring%20Boot-3.2.0-brightgreen.svg)
![Java](https://img.shields.io/badge/Java-17-blue.svg)
![Maven](https://img.shields.io/badge/Maven-3.6+-red.svg)
![MQTT](https://img.shields.io/badge/MQTT-Paho-green.svg)
![Redis](https://img.shields.io/badge/Redis-Lettuce-red.svg)

DWM API Server là hệ thống backend xử lý dữ liệu công nghiệp được chuyển đổi từ **Python FastAPI** sang **Spring Boot Java**, tối ưu cho việc deploy lên **Tomcat** với độ ổn định cao.

## 📋 Tổng quan

### Chức năng chính
- **MQTT Data Processing** - Thu thập và xử lý dữ liệu thời gian thực từ thiết bị DWM
- **Machine Dashboard** - Giám sát trạng thái và thống kê máy móc
- **Redis Deduplication** - Loại bỏ dữ liệu trùng lặp với atomic operation
- **RESTful API** - Cung cấp API chuẩn REST cho frontend

### Ưu điểm so với Python version
✅ **Ổn định hơn** - Không cần NSSM, chạy native trên Tomcat  
✅ **Performance cao** - JVM optimization, multi-threading  
✅ **Type-safe** - Compile-time checking  
✅ **Enterprise-ready** - Spring Boot ecosystem  
✅ **Dễ deploy** - Chỉ cần 1 file WAR  

---

## 🏗️ Kiến trúc

```
├── src/main/java/com/dwm/apiserver/
│   ├── DwmApiServerApplication.java    # Main entry point
│   ├── config/
│   │   └── CorsConfig.java             # CORS configuration
│   ├── controller/
│   │   ├── MqttController.java         # MQTT API endpoints
│   │   ├── MachineDashboardController.java # Dashboard API
│   │   └── RootController.java         # Root endpoint
│   ├── service/
│   │   ├── MqttService.java            # MQTT connection & handling
│   │   ├── MqttDataParserService.java  # FD09 data parsing
│   │   ├── MachineDashboardService.java # Dashboard logic
│   │   └── RedisDeduplicationService.java # Redis deduplication
│   ├── repository/
│   │   ├── DwmRawDataRepository.java
│   │   └── DwmMachineListRepository.java
│   ├── model/
│   │   ├── DwmRawData.java             # JPA Entity
│   │   └── DwmMachineList.java         # JPA Entity
│   └── dto/
│       ├── MachineDataResponse.java
│       ├── TotalMachineDataResponse.java
│       ├── MqttStatusResponse.java
│       ├── MqttMessageResponse.java
│       └── ParsedMessageResponse.java
│
├── src/main/resources/
│   └── application.properties          # Configuration
│
└── pom.xml                             # Maven dependencies
```

---

## 🚀 Cài đặt và Chạy

### Yêu cầu hệ thống
- **JDK 17+** (khuyến nghị OpenJDK 17 hoặc Oracle JDK 17)
- **Maven 3.6+**
- **MSSQL Server** (với ODBC Driver)
- **Redis Server** (bắt buộc)
- **MQTT Broker** (Mosquitto, EMQX, etc.)
- **Apache Tomcat 10+** (cho production deployment)

### Bước 1: Clone và cấu hình

```bash
cd backend
```

### Bước 2: Cấu hình Database, MQTT, Redis

Chỉnh sửa file `src/main/resources/application.properties`:

```properties
# Database
spring.datasource.url=jdbc:sqlserver://10.32.10.203:1433;databaseName=your_db
spring.datasource.username=your_username
spring.datasource.password=your_password

# Redis
spring.data.redis.host=localhost
spring.data.redis.port=6379

# MQTT
mqtt.broker.url=tcp://127.0.0.1:1883
mqtt.username=api_mqtt
mqtt.password=HLIG@mqtt369
mqtt.topic=dwm_data
```

### Bước 3: Build project

```bash
# Clean và build WAR file
mvn clean package
mvn clean package -DskipTests
mvn spring-boot:run
# WAR file sẽ được tạo tại: target/dwm-api-server.war
```

### Bước 4: Chạy ứng dụng

#### 🔵 **Development Mode (Standalone)**

```bash
# VS Code Terminal (khuyến nghị - tự động UTF-8)
# Terminal đã được cấu hình tự động UTF-8 trong .vscode/settings.json
# Chỉ cần chạy:
mvn spring-boot:run

# Hoặc dùng Debug (F5) - encoding tự động

# Terminal ngoài (Command Prompt/PowerShell)
# Cách 1: Dùng script (tự động set UTF-8)
run.bat

# Cách 2: Set encoding thủ công
chcp 65001
mvn spring-boot:run

# Cách 3: Chạy JAR
chcp 65001
java -Dfile.encoding=UTF-8 -jar target/dwm-api-server.war

# Ứng dụng sẽ chạy tại: http://localhost:8089
```

**Lưu ý về Encoding:**
- **VS Code:** Terminal đã được cấu hình tự động UTF-8 trong `.vscode/settings.json`
  - Chỉ cần chạy `mvn spring-boot:run` - không cần `chcp 65001`
  - Hoặc dùng Debug (F5) - encoding tự động
  
- **Terminal ngoài:** Cần set encoding trước:
  - Chạy `chcp 65001` trước khi chạy ứng dụng
  - Hoặc dùng script `run.bat` (tự động set encoding)
  
- **JVM encoding:** Đã được cấu hình tự động trong `pom.xml` (không cần set thủ công)

#### 🟢 **Production Mode (Tomcat)**

1. **Copy WAR file vào Tomcat:**
```bash
# Windows
copy target\dwm-api-server.war C:\apache-tomcat-10\webapps\

# Linux
cp target/dwm-api-server.war /opt/tomcat/webapps/
```

2. **Khởi động Tomcat:**
```bash
# Windows
C:\apache-tomcat-10\bin\startup.bat

# Linux
/opt/tomcat/bin/startup.sh
```

3. **Truy cập:**
```
http://localhost:8080/dwm-api-server/
```

---

## 📚 API Endpoints

### MQTT APIs

| Method | Endpoint | Mô tả |
|--------|----------|-------|
| GET | `/mqtt/status` | Trạng thái kết nối MQTT/Redis |
| GET | `/mqtt/messages?limit=10` | 10 tin nhắn MQTT gần nhất |
| GET | `/mqtt/parsed-messages?limit=10` | Lịch sử phân tích dữ liệu |
| POST | `/mqtt/publish?topic=xxx&message=xxx` | Gửi tin nhắn lên MQTT |

### Machine Dashboard APIs

| Method | Endpoint | Mô tả |
|--------|----------|-------|
| GET | `/dwm-Dashboard/machines-daily` | Thống kê tất cả máy trong ngày |
| GET | `/dwm-Dashboard/machines-daily/{line_id}` | Lọc theo chuyền sản xuất |
| GET | `/dwm-Dashboard/total-machines-daily` | Tổng hợp tất cả máy |
| GET | `/dwm-Dashboard/total-machines-daily/{line_id}` | Tổng hợp theo chuyền |

### Test API

```bash
# Kiểm tra trạng thái
curl http://localhost:8089/

# Kiểm tra MQTT status
curl http://localhost:8089/mqtt/status

# Lấy dữ liệu dashboard
curl http://localhost:8089/dwm-Dashboard/machines-daily
```

---

## 🔧 Chạy như Windows Service (NSSM)

Nếu muốn chạy standalone WAR như service (không dùng Tomcat):

```bash
# Download NSSM: https://nssm.cc/download

# Install service
nssm install DWMAPIServer

# Configuration
Path: C:\Program Files\Java\jdk-17\bin\java.exe
Arguments: -jar "D:\path\to\dwm-api-server.war"
Startup directory: D:\path\to\

# Start service
nssm start DWMAPIServer
```

**Lưu ý:** Với Spring Boot, ổn định hơn nhiều so với Python + uvicorn workers!

---

## 🔍 So sánh với Python Version

| Tính năng | Python FastAPI | Spring Boot Java |
|-----------|---------------|------------------|
| **Deployment** | uvicorn + NSSM | Tomcat (native) |
| **Stability** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Performance** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Type Safety** | Runtime | Compile-time |
| **Memory Usage** | ~200MB | ~300MB |
| **Startup Time** | 2-3s | 5-8s |
| **Enterprise Support** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |

---

## 📊 Monitoring & Logs

### Log location

```bash
# Console output (nếu chạy standalone)
java -jar dwm-api-server.war

# Tomcat logs
# Windows: C:\apache-tomcat-10\logs\catalina.out
# Linux: /opt/tomcat/logs/catalina.out
```

### Health Check Endpoints

```bash
# Application status
curl http://localhost:8089/

# MQTT connection status
curl http://localhost:8089/mqtt/status
```

---

## 🛠️ Troubleshooting

### 1. **Database connection failed**
```bash
# Kiểm tra connection string trong application.properties
# Đảm bảo MSSQL Server đang chạy
# Kiểm tra firewall port 1433
```

### 2. **Redis connection failed**
```bash
# Kiểm tra Redis service
redis-cli ping  # Phải trả về PONG

# Windows: Khởi động Redis
redis-server.exe
```

### 3. **MQTT connection failed**
```bash
# Kiểm tra MQTT broker
mqtt-cli sub -t dwm_data -h 127.0.0.1 -p 1883

# Kiểm tra username/password trong application.properties
```

### 4. **Port 8089 đã được sử dụng**
```properties
# Đổi port trong application.properties
server.port=8090
```

---

## 📦 Build & Deploy Script

### Windows Deploy Script

Tạo file `deploy.bat`:

```batch
@echo off
echo ===== Building DWM API Server =====
call mvn clean package -DskipTests

echo ===== Stopping Tomcat =====
call C:\apache-tomcat-10\bin\shutdown.bat

timeout /t 5

echo ===== Deploying WAR =====
copy /Y target\dwm-api-server.war C:\apache-tomcat-10\webapps\

echo ===== Starting Tomcat =====
call C:\apache-tomcat-10\bin\startup.bat

echo ===== Deployment Complete =====
pause
```

### Linux Deploy Script

Tạo file `deploy.sh`:

```bash
#!/bin/bash
echo "===== Building DWM API Server ====="
mvn clean package -DskipTests

echo "===== Stopping Tomcat ====="
/opt/tomcat/bin/shutdown.sh
sleep 5

echo "===== Deploying WAR ====="
cp target/dwm-api-server.war /opt/tomcat/webapps/

echo "===== Starting Tomcat ====="
/opt/tomcat/bin/startup.sh

echo "===== Deployment Complete ====="
```

---

## 🤝 Migration từ Python

Project này là **100% equivalent** với Python version:
- ✅ Tất cả API endpoints giữ nguyên
- ✅ Response format giống hệt
- ✅ Database schema không đổi
- ✅ MQTT protocol tương thích
- ✅ Redis deduplication logic giống

**Frontend không cần thay đổi gì!**

---

## 📞 Support

Nếu gặp vấn đề:
1. Kiểm tra logs trong Tomcat
2. Verify database/MQTT/Redis connection
3. Check port conflicts
4. Review application.properties configuration

---

**DWM API Server - Spring Boot Edition** ⚡  
*Production-ready, Enterprise-grade, Rock-solid!*

