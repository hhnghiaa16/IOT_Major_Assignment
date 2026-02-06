# 🏠 AIoT Trợ Lý AI Giọng Nói

## ✨ Tính năng

| Tính năng | Mô tả |
|-----------|-------|
| 🎤 **Voice Control** | Điều khiển thiết bị bằng giọng nói tiếng Việt |
| 🤖 **AI Assistant** | Trợ lý AI với Function Calling tự động thực thi lệnh |
| 📊 **Dashboard** | Giám sát nhiệt độ, độ ẩm real-time với biểu đồ |
| 📱 **Device Management** | Quản lý thiết bị Master-Slave, cấu hình virtual pins |
| 🔄 **OTA Update** | Cập nhật firmware ESP32 từ xa qua web |
| 🔐 **Authentication** | Đăng nhập JWT với phân quyền Admin/Viewer |
| 💬 **AI Chat** | Chat với AI, tùy chỉnh tính cách và giọng nói |

---

## 🏗 Kiến trúc hệ thống

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   ESP32 Master  │────▶│  Backend API    │◀────│   Web Frontend  │
│  (Mic + Speaker)│     │  (FastAPI)      │     │   (React)       │
└────────┬────────┘     └────────┬────────┘     └─────────────────┘
         │                       │
         │ MQTT                  │ REST/WebSocket
         │                       │
┌────────▼────────┐     ┌────────▼────────┐
│   ESP32 Slave   │     │   AI Services   │
│  (Sensors/LED)  │     │ (STT/LLM/TTS)   │
└─────────────────┘     └─────────────────┘
```

**Voice Pipeline:**
```
User Voice → ESP32 Mic → WebSocket → STT (Whisper) → LLM (Grok) → Device Control → TTS → ESP32 Speaker
```

## 🛠 Cài đặt

### 1. Clone repository

```bash
git clone https://github.com/your-repo/IOT_Major_Assignment.git
cd IOT_Major_Assignment/Code
```

### 2. Cài đặt Backend

```bash
cd iot-backend

# Tạo virtual environment
python -m venv .venv

# Kích hoạt (Windows)
.venv\Scripts\activate

# Kích hoạt (Linux/Mac)
source .venv/bin/activate

# Cài đặt dependencies
pip install -r requirements.txt
```

### 3. Cài đặt Frontend

```bash
cd client

# Cài đặt dependencies
npm install
```

### 4. Cài đặt Firmware ESP32

1. Mở Arduino IDE hoặc PlatformIO
2. Cài đặt ESP32 board support
3. Copy các thư viện từ `client/thuvien/` vào Arduino libraries
4. Mở `firmware_master/firmware_master.ino` hoặc `firmware_client/firmware_client.ino`
5. Cấu hình WiFi và upload

---

## ⚙ Cấu hình

### Backend (.env)

Tạo file `iot-backend/.env`:

```env
# Database
DATABASE_URL=sqlite:///./iot.db

# MQTT Broker
MQTT_BROKER_HOST=localhost
MQTT_BROKER_PORT=1883

# AI Services
GROQ_API_KEY=your_groq_api_key
OPENROUTER_API_KEY=your_openrouter_api_key

# Optional: Proxy
PROXY_URL=
PROXY_USERNAME=
PROXY_PASSWORD=

# JWT
SECRET_KEY=your_secret_key
ALGORITHM=HS256
ACCESS_TOKEN_EXPIRE_MINUTES=30
```

### ESP32 Firmware

Trong `firmware_master.ino`, cập nhật:

```cpp
#define CLIENT_ID "your_device_token"
#define version  "Master_1.0.0"
#define OTA_SERVER_URL "http://your_server:8000/ota/get_info_update"
```

WiFi credentials trong `settings.h` hoặc qua NVS.

---

## 🚀 Chạy ứng dụng

### 1. Khởi động Backend

```bash
cd iot-backend

# Kích hoạt virtual environment
.venv\Scripts\activate  # Windows
source .venv/bin/activate  # Linux/Mac

# Chạy server
uvicorn app.main:app --host 0.0.0.0 --port 8000 --reload
```

Backend sẽ chạy tại: `http://localhost:8000`
- API Docs: `http://localhost:8000/docs`
- ReDoc: `http://localhost:8000/redoc`

### 2. Khởi động Frontend

```bash
cd client

npm start
```

Frontend sẽ chạy tại: `http://localhost:3000`

### 3. Khởi động MQTT Broker

Gọi API để start MQTT:
```bash
curl -X POST http://localhost:8000/mqtt/start
```

---

## 📚 API Documentation

### Authentication

| Method | Endpoint | Mô tả |
|--------|----------|-------|
| POST | `/auth/register` | Đăng ký tài khoản |
| POST | `/auth/login` | Đăng nhập |
| GET | `/auth/me` | Thông tin user hiện tại |

### Devices

| Method | Endpoint | Mô tả |
|--------|----------|-------|
| GET | `/devices/` | Danh sách thiết bị |
| POST | `/devices/` | Thêm thiết bị mới |
| PUT | `/devices/{id}` | Cập nhật thiết bị |
| DELETE | `/devices/{id}` | Xóa thiết bị |

### MQTT

| Method | Endpoint | Mô tả |
|--------|----------|-------|
| GET | `/mqtt/status` | Trạng thái MQTT broker |
| POST | `/mqtt/start` | Khởi động broker |
| POST | `/mqtt/stop` | Dừng broker |
| POST | `/mqtt/publish` | Publish message |

### AI Chat

| Method | Endpoint | Mô tả |
|--------|----------|-------|
| POST | `/ai/chat` | Gửi tin nhắn chat |
| GET | `/ai/config_ai` | Lấy cấu hình AI |
| PUT | `/ai/config_ai` | Cập nhật cấu hình AI |

### OTA

| Method | Endpoint | Mô tả |
|--------|----------|-------|
| POST | `/ota/upload` | Upload firmware mới |
| GET | `/ota/get_info_update` | Kiểm tra bản cập nhật |
| GET | `/ota/check-info-ota` | Thông tin OTA của devices |

### WebSocket

| Endpoint | Mô tả |
|----------|-------|
| `/ws/mqtt` | MQTT bridge cho real-time updates |
| `/audio_stream/ws/{client_id}` | Audio streaming cho voice |

---
