# HTTP OTA firmware qua MQTT (ANDON-ESP32-MP3)

Tài liệu ghi lại cách cập nhật firmware ESP32 **không cần USB** và **không cần PC cùng WiFi xưởng**: board tải file `.bin` qua HTTP sau khi nhận lệnh trên topic MQTT `updateFirmware`.
pio run

python -m http.server 8000
chú ý up trong EMQTT toppic phải là updateFirmware
updateFirmware
cập nhật tất cả là
`http://10.32.10.205:8000/firmware.bin`
còn cập nhật từng máy thì thêm địa chỉ
`E83DC19314C8|http://10.32.10.205:8000/firmware.bin`

## Điều kiện cần có

- Firmware trên ESP đã có code HTTP OTA + subscribe `updateFirmware` (xem `src/main.cpp`).
- ESP kết nối được **MQTT broker** (trong code: host/port giống `mqttServer` / `mqttPort`).
- ESP **mở được URL HTTP** của file firmware (cùng mạng nội bộ hoặc routing cho phép tới máy chủ HTTP).
- File phục vụ phải là **`firmware.bin`** do PlatformIO build ra (đúng board/env), không dùng nhầm file khác.

## Bước 1 — Build firmware

Trong thư mục `ANDON-ESP32-MP3`:

```bash
pio run
```

File nạp nằm tại:

`.pio/build/esp32-s3-devkitc-1/firmware.bin`

Copy file này lên máy chủ HTTP (ví dụ đặt tên `firmware.bin`).

## Bước 2 — Phục vụ file qua HTTP
 python -m http.server 8000   
ESP dùng thư viện `HTTPUpdate`: cần URL dạng `http://.../firmware.bin` (HTTPS cũng hỗ trợ với `setInsecure()` trong code).

Ví dụ máy chủ `10.32.10.205`, cổng `8000`, đường dẫn `/firmware.bin`:

- URL đầy đủ: `http://10.32.10.205:8000/firmware.bin`

Có thể dùng bất kỳ cách nào miễn **GET** trả về đúng nội dung file nhị phân:

- `python -m http.server 8000` trong thư mục chứa `firmware.bin` (chỉ dùng thử nghiệm).
- Nginx, IIS, hoặc static file của ứng dụng backend.

Kiểm tra nhanh trên PC (cùng mạng có thể tới server):

```bash
curl -I http://10.32.10.205:8000/firmware.bin
```

Kỳ vọng HTTP `200` và `Content-Length` > 0.

## Bước 3 — Topic và payload MQTT

| Mục | Giá trị trong firmware hiện tại |
|-----|----------------------------------|
| Topic | `updateFirmware` |
| Tên thiết bị (lọc) | Theo macro `MP3_DEVICE_NAME` trong `main.cpp` (ví dụ `MVT3`) |


Hai dạng payload:

1. **Chỉ cập nhật một board** (khuyến nghị):  
   `TênBoard|URL`  
   Ví dụ:  
   `MVT3|http://10.32.10.205:8000/firmware.bin`


2. **Payload chỉ là URL** (không có `|`): mọi board đang subscribe `updateFirmware` đều có thể chạy OTA — chỉ dùng khi bạn chủ động muốn cập nhật hàng loạt.

## Bước 4 — Gửi lệnh bằng MQTTX

1. Tạo kết nối tới broker (cùng host/port với cấu hình ESP trong `main.cpp`).
2. Đăng nhập user/password MQTT **trùng** với ESP (xem `mqttUser` / `mqttPassword` trong `main.cpp`).
3. **Publish**:
   - Topic: `updateFirmware`
   - Payload: `MVT3|http://10.32.10.205:8000/firmware.bin` (đổi `MVT3` hoặc URL nếu khác môi trường).
   - QoS: thường dùng `0` hoặc `1` tùy broker.

Sau khi gửi, ESP sẽ: ngắt MQTT tạm thời → tải `.bin` → ghi flash → **tự khởi động lại**. Nếu lỗi tải/ghi, firmware sẽ kết nối lại MQTT (xem log Serial).

## Kiểm tra trên Serial (tuỳ chọn)

Cắm USB mở monitor `115200` để thấy log kiểu `HTTP OTA: start`, phần trăm, `done, reboot...` hoặc mã lỗi.

## Lỗi thường gặp

| Hiện tượng | Hướng xử lý |
|------------|-------------|
| Không OTA sau khi publish | Kiểm tra đúng topic; payload `TênBoard|` phải khớp `MP3_DEVICE_NAME`; ESP có online trên broker không. |
| OTA fail / timeout HTTP | ESP không route tới IP:port máy chủ; firewall chặn; URL sai; server không chạy. |
| HTTP 404 | Sai đường dẫn hoặc file chưa copy đúng chỗ. |
| Nhiều board cùng flash | Dùng nhầm payload chỉ URL — chuyển sang dạng `MVT3|...` cho từng thiết bị. |

## Ghi chú bảo mật

Ai publish được vào `updateFirmware` có thể ép board nạp firmware từ URL — nên cấu hình **ACL trên EMQX** (chỉ user/service được phép publish topic này). Về sau có thể bổ sung chữ ký file hoặc HTTPS có xác thực chứng chỉ thay vì `setInsecure()`.

## Tham chiếu code

- Topic OTA: `mqttOtaTopic` (`updateFirmware`).
- Tên board: `MP3_DEVICE_NAME` / `mp3DeviceName`.
- OTA Arduino qua LAN (PlatformIO `espota`): vẫn dùng env `esp32-s3-devkitc-1-ota` trong `platformio.ini` khi PC cùng mạng với ESP — khác với luồng MQTT+HTTP này.
