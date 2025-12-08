#include "MicRecorder.h"

// Global instance pointer for ISR and callbacks
MicRecorder* micRecorderInstance = nullptr;
volatile bool isDeviceRecording = false;

// ======= Constructor & Destructor =======
MicRecorder::MicRecorder() {
    state = RECORDER_IDLE;
    buttonPressed = false;
    wsConnected = false;
    audioBuffer = nullptr;
    bufferIndex = 0;
    lastSendTime = 0;
    recordStartTime = 0;
    chunksRecorded = 0;
    chunksSent = 0;
    
    micRecorderInstance = this;
}

MicRecorder::~MicRecorder() {
    stopRecording();
    deinitI2S();
    
    if (audioBuffer) {
        free(audioBuffer);
        audioBuffer = nullptr;
    }
    
    micRecorderInstance = nullptr;
}

// ======= Initialization =======
bool MicRecorder::begin() {
    Serial.println("[MicRecorder] Initializing...");
    
    // Allocate audio buffer
    audioBuffer = (uint8_t*)malloc(AUDIO_BUFFER_SIZE);
    if (!audioBuffer) {
        Serial.println("[MicRecorder] ERROR: Failed to allocate audio buffer!");
        state = RECORDER_ERROR;
        return false;
    }
    
    // Setup record button
    pinMode(RECORD_BUTTON_PIN, BUTTON_ACTIVE_LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
    
    Serial.printf("[MicRecorder] Initialized successfully\n");
    Serial.printf("[MicRecorder] Mic I2S Pins - WS:%d, SCK:%d, SD:%d\n", 
                  MIC_I2S_WS, MIC_I2S_SCK, MIC_I2S_SD);
    Serial.printf("[MicRecorder] Record Button: GPIO%d (%s)\n", 
                  RECORD_BUTTON_PIN, BUTTON_ACTIVE_LOW ? "PULL-UP" : "PULL-DOWN");
    
    // Load WebSocket URL from NVS (with default empty string if not exists)
    Settings wsSettings("mqtt", false);
    wsServerUrl = wsSettings.getString("url", "");  // ← Thêm default value
    if (wsServerUrl.length() > 0) {
        Serial.printf("[MicRecorder] WebSocket URL: %s\n", wsServerUrl.c_str());
    } else {
        Serial.println("[MicRecorder] WebSocket URL: Not set (will use default)");
    }
    
    state = RECORDER_IDLE;
    return true;
}

// ======= I2S Functions =======
bool MicRecorder::initI2S() {
    Serial.println("[MicRecorder] Configuring I2S for INMP441...");
    
    // I2S configuration for INMP441 microphone
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // INMP441 outputs on left channel
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    
    // I2S pin configuration for microphone
    i2s_pin_config_t pin_config = {
        .bck_io_num = MIC_I2S_SCK,
        .ws_io_num = MIC_I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,  // Not used for input
        .data_in_num = MIC_I2S_SD
    };
    
    // Install and configure I2S driver
    esp_err_t err = i2s_driver_install(I2S_MIC_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[MicRecorder] ERROR: I2S driver install failed: %d\n", err);
        return false;
    }
    
    err = i2s_set_pin(I2S_MIC_PORT, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[MicRecorder] ERROR: I2S set pin failed: %d\n", err);
        i2s_driver_uninstall(I2S_MIC_PORT);
        return false;
    }
    
    // Clear DMA buffer
    i2s_zero_dma_buffer(I2S_MIC_PORT);
    
    Serial.println("[MicRecorder] I2S initialized for microphone");
    return true;
}

void MicRecorder::deinitI2S() {
    i2s_driver_uninstall(I2S_MIC_PORT);
    Serial.println("[MicRecorder] I2S deinitialized");
}

// ======= WebSocket Functions =======
void MicRecorder::webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    if (!micRecorderInstance) return;
    
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("[MicRecorder] WebSocket disconnected");
            micRecorderInstance->wsConnected = false;
            break;
            
        case WStype_CONNECTED:
            Serial.println("[MicRecorder] WebSocket connected");
            micRecorderInstance->wsConnected = true;
            break;
            
        case WStype_TEXT:
            // Received text message (STT result from server)
            Serial.printf("[MicRecorder] Received: %s\n", payload);
            break;
            
        case WStype_BIN:
            // Binary data received (not expected)
            break;
            
        case WStype_ERROR:
            Serial.println("[MicRecorder] WebSocket error");
            micRecorderInstance->state = RECORDER_ERROR;
            break;
            
        case WStype_PING:
        case WStype_PONG:
            break;
    }
}

bool MicRecorder::connectWebSocket(const String& url) {
    Serial.printf("[MicRecorder] Connecting to WebSocket: %s\n", url.c_str());
    // ws://localhost:8000/audio_stream/ws/
    // Parse URL: ws://host:port/path
    String urlCopy = url;
    urlCopy.replace("ws://", "");
    urlCopy.replace("wss://", "");
    
    int colonPos = urlCopy.indexOf(':');
    int slashPos = urlCopy.indexOf('/');
    
    String host;
    uint16_t port = 80;
    String path = "/";
    
    if (colonPos > 0 && slashPos > colonPos) {
        host = urlCopy.substring(0, colonPos);
        port = urlCopy.substring(colonPos + 1, slashPos).toInt();
        path = urlCopy.substring(slashPos);
    } else if (slashPos > 0) {
        host = urlCopy.substring(0, slashPos);
        path = urlCopy.substring(slashPos);
    } else {
        host = urlCopy;
    }
    Serial.printf("[MicRecorder] Host: %s, Port: %d, Path: %s\n", host.c_str(), port, path.c_str());
    
    webSocket.begin(host, port, path);
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(1000);
    
    // Wait for connection (with timeout)
    unsigned long startTime = millis();
    while (!wsConnected && (millis() - startTime < 1000)) {
        webSocket.loop();
        delay(10);
    }
    
    return wsConnected;
}

void MicRecorder::disconnectWebSocket() {
    webSocket.disconnect();
    wsConnected = false;
    Serial.println("[MicRecorder] WebSocket disconnected");
}

void MicRecorder::sendAudioChunk(uint8_t* data, size_t len) {
    if (wsConnected && len > 0) {
        webSocket.sendBIN(data, len);
        chunksSent++;
    }
}

// ======= Recording Control =======
bool MicRecorder::startRecording(const String& serverUrl, const String& clientId) {
    if (state == RECORDER_RECORDING) {
        Serial.println("[MicRecorder] Already recording!");
        return false;
    }
    
    Serial.println("[MicRecorder] Starting recording...");
    
    state = RECORDER_CONNECTING;
    this->clientId = clientId;
    
    // Build WebSocket URL with client ID
    String wsUrl = serverUrl;
    if (!wsUrl.endsWith("/")) {
        wsUrl += "/";
    }
    wsUrl += clientId;
    
    // Initialize I2S
    if (!initI2S()) {
        state = RECORDER_ERROR;
        return false;
    }
    
    // Connect to WebSocket
    if (!connectWebSocket(wsUrl)) {
        Serial.println("[MicRecorder] Failed to connect WebSocket!");
        deinitI2S();
        state = RECORDER_ERROR;
        return false;
    }
    
    // Reset buffers and counters
    bufferIndex = 0;
    chunksRecorded = 0;
    chunksSent = 0;
    recordStartTime = millis();
    
    state = RECORDER_RECORDING;
    isDeviceRecording = true;
    
    Serial.println("[MicRecorder] ✓ Recording started!");
    return true;
}

void MicRecorder::stopRecording() {
    if (state != RECORDER_RECORDING && state != RECORDER_CONNECTING) {
        return;
    }
    
    Serial.println("[MicRecorder] Stopping recording...");
    state = RECORDER_STOPPING;
    
    // Send remaining buffer
    if (bufferIndex > 0) {
        sendAudioChunk(audioBuffer, bufferIndex);
        bufferIndex = 0;
    }
    
    // Small delay to ensure last chunk is sent
    delay(100);
    
    // Disconnect and cleanup
    disconnectWebSocket();
    deinitI2S();
    
    unsigned long duration = millis() - recordStartTime;
    Serial.printf("[MicRecorder] ✓ Recording stopped!\n");
    Serial.printf("[MicRecorder] Duration: %lu ms, Chunks: %u recorded, %u sent\n", 
                  duration, chunksRecorded, chunksSent);
    
    state = RECORDER_IDLE;
    isDeviceRecording = false;
}

// ======= Update (call in loop/task) =======
void MicRecorder::update() {
    // Handle WebSocket events
    if (wsConnected || state == RECORDER_CONNECTING) {
        webSocket.loop();
    }
    
    // Read and send audio data
    if (state == RECORDER_RECORDING && wsConnected) {
        size_t bytesRead = 0;
        uint8_t tempBuffer[I2S_READ_LEN];
        
        // Read from I2S DMA buffer
        esp_err_t err = i2s_read(I2S_MIC_PORT, tempBuffer, I2S_READ_LEN, &bytesRead, pdMS_TO_TICKS(10));
        
        if (err == ESP_OK && bytesRead > 0) {
            chunksRecorded++;
            
            // Gửi trực tiếp từ tempBuffer - không cần copy qua audioBuffer
            // Vì I2S_READ_LEN = 1024 bytes = đúng kích thước API yêu cầu
            sendAudioChunk(tempBuffer, bytesRead);
            chunksSent++;
        }
    }
}


// ======= State Functions =======
bool MicRecorder::isRecording() {
    return (state == RECORDER_RECORDING);
}

RecorderState MicRecorder::getState() {
    return state;
}

// ======= Button Handlers =======
void MicRecorder::onButtonPressed() {
    buttonPressed = true;
    Serial.println("[MicRecorder] Button pressed");
}

void MicRecorder::onButtonReleased() {
    buttonPressed = false;
    Serial.println("[MicRecorder] Button released");
}

// ======= Configuration =======
void MicRecorder::setWebSocketUrl(const String& url) {
    wsServerUrl = url;
    Serial.printf("[MicRecorder] WebSocket URL set: %s\n", url.c_str());
}

String MicRecorder::getWebSocketUrl() {
    return wsServerUrl;
}

// ======= Debug =======
void MicRecorder::printStatus() {
    Serial.println("\n===== MicRecorder Status =====");
    Serial.printf("State: ");
    switch(state) {
        case RECORDER_IDLE: Serial.println("IDLE"); break;
        case RECORDER_CONNECTING: Serial.println("CONNECTING"); break;
        case RECORDER_RECORDING: Serial.println("RECORDING"); break;
        case RECORDER_STOPPING: Serial.println("STOPPING"); break;
        case RECORDER_ERROR: Serial.println("ERROR"); break;
    }
    Serial.printf("WebSocket URL: %s\n", wsServerUrl.c_str());
    Serial.printf("Client ID: %s\n", clientId.c_str());
    Serial.printf("WS Connected: %s\n", wsConnected ? "YES" : "NO");
    Serial.printf("Chunks Recorded: %u\n", chunksRecorded);
    Serial.printf("Chunks Sent: %u\n", chunksSent);
    Serial.printf("I2S Pins - WS:%d, SCK:%d, SD:%d\n", MIC_I2S_WS, MIC_I2S_SCK, MIC_I2S_SD);
    Serial.printf("Button Pin: GPIO%d\n", RECORD_BUTTON_PIN);
    Serial.println("==============================\n");
}
