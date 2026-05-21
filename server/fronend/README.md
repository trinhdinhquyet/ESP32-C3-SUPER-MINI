# 🚀 Deploy lên Tomcat - Subdirectory /dwm/
npm install
npm run dev
## 🔀 Tương thích Tomcat

Dự án hỗ trợ cả **Tomcat 9 (JDK 8)** và **Tomcat 11 (JDK 17)**:

| Tomcat Version | JDK | Servlet API | JSP | Namespace |
|---------------|-----|-------------|-----|-----------|
| Tomcat 9 | JDK 8 | Servlet 4.0 | JSP 2.3 | javax.* |
| Tomcat 11 | JDK 17+ | Servlet 6.1 | JSP 4.0 | jakarta.* |

✅ File `WEB-INF/web.xml` đã được cấu hình với Jakarta EE namespace cho Tomcat 11

## ✅ Cấu hình đúng cho Tomcat

### 🔧 Cách deploy:

1. **Build project với cấu hình Tomcat:**
   ```bash
npm run build:tomcat
   ```
   Hoặc double-click `build-for-tomcat.bat`

2. **Copy toàn bộ thư mục `dist/` vào:**
   ```
   [TOMCAT_HOME]/webapps/dwm/
   ```

3. **Cấu trúc thư mục sau khi copy:**
   ```
   [TOMCAT_HOME]/webapps/dwm/
   ├── index.html
   ├── favicon.ico
   ├── background.jpg
   ├── logo.png
   ├── logo.webp
   ├── logotron.png
   ├── WEB-INF/
   │   └── web.xml          (cho Tomcat 11 - Jakarta EE)
   └── assets/
       ├── index-CERgLcU7.js
       ├── index-Deqf6PPj.css
       ├── vendor-DtSzv8NO.js
       └── chart-Co7ClN-s.js
   ```

4. **Truy cập ứng dụng:**
   ```
   http://10.32.10.70:8888/dwm/
   ```

## 🎯 Vấn đề đã được fix:

### ❌ Trước đây:
- Request URL: `http://10.32.10.70:8888/assets/index-BlE0SX6B.css` → 404
- Request URL: `http://10.32.10.70:8888/assets/index-CsOqkr59.js` → 404

### ✅ Bây giờ:
- Request URL: `http://10.32.10.70:8888/dwm/assets/index-Deqf6PPj.css` → ✅
- Request URL: `http://10.32.10.70:8888/dwm/assets/index-CERgLcU7.js` → ✅

## 📋 Scripts có sẵn:

- `npm run build:tomcat` - Build cho Tomcat subdirectory
- `build-for-tomcat.bat` - Build tự động cho Tomcat

## 🔄 Quy trình deploy:

1. Chạy `npm run build:tomcat`
2. Copy thư mục `dist/` → `[TOMCAT_HOME]/webapps/dwm/`
3. Restart Tomcat (nếu cần)
4. Truy cập `http://10.32.10.70:8888/dwm/`

## ⚠️ Lưu ý:

- Đảm bảo copy **toàn bộ** thư mục `dist/` (bao gồm cả `assets/` và `WEB-INF/`)
- Không cần rename gì thêm
- Các đường dẫn đã được cấu hình đúng với prefix `/dwm/`
- File `WEB-INF/web.xml` sử dụng Jakarta EE namespace (tương thích Tomcat 11)
- Tomcat 9 vẫn có thể chạy với web.xml này (backward compatible)

## 🆕 Tomcat 11 (JDK 17) Support:

Dự án đã được cấu hình để hỗ trợ Tomcat 11:

- ✅ **Jakarta EE 11** namespace (`jakarta.*`)
- ✅ **Servlet 6.1** API
- ✅ **JSP 4.0** specification
- ✅ File `WEB-INF/web.xml` với schema Jakarta EE 6.0
- ✅ MIME types cho các file JavaScript modules (`.mjs`)
- ✅ UTF-8 encoding cho JSP files

### Yêu cầu:
- **JDK 17** trở lên
- **Tomcat 11.x**

### Kiểm tra sau khi deploy:
1. Truy cập `http://[YOUR_SERVER]:8888/dwm/` - phải load được trang chủ
2. Kiểm tra backend API: `http://[YOUR_SERVER]:8089/api/server-time` - phải trả về JSON với thời gian server
3. Kiểm tra browser console - không có lỗi 404 cho các file `.js`, `.css`
