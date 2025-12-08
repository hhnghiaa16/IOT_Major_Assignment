#ifndef MIC_RECORDER_H
#define MIC_RECORDER_H

#include <Arduino.h>
#include <driver/i2s.h>
#include <WebSocketsClient.h>
#include "settings.h"
// ======= Pin Definitions (User configurable) =======
// INMP441 Microphone I2S pins
#ifndef MIC_I2S_WS
#define MIC_I2S_WS    15    // Word Select (LRCLK)
#endif
#ifndef MIC_I2S_SCK
#define MIC_I2S_SCK   14    // Serial Clock (BCLK)
#endif
#ifndef MIC_I2S_SD
#define MIC_I2S_SD    32    // Serial Data (DOUT)
#endif

// Record button pin
#ifndef RECORD_BUTTON_PIN
#define RECORD_BUTTON_PIN 33  // Default GPIO33
#endif
#ifndef BUTTON_ACTIVE_LOW
#define BUTTON_ACTIVE_LOW true  // true = Pull-up, false = Pull-down
#endif

// MAX98357A Speaker I2S pins (to avoid conflict)
#ifndef SPEAKER_I2S_BCLK
#define SPEAKER_I2S_BCLK  26
#endif
#ifndef SPEAKER_I2S_LRC
#define SPEAKER_I2S_LRC   25
#endif
#ifndef SPEAKER_I2S_DOUT
#define SPEAKER_I2S_DOUT  22
#endif

// ======= Audio Configuration =======
#define I2S_SAMPLE_RATE     16000   // 16kHz for STT
#define I2S_SAMPLE_BITS     16      // 16-bit audio
#define I2S_CHANNEL_NUM     1       // Mono
#define I2S_READ_LEN        1024    // 1024 bytes = 512 samples (32ms at 16kHz)
#define AUDIO_BUFFER_SIZE   1024    // Send immediately after I2S read (512 samples = 1024 bytes)

// I2S port numbers
#define I2S_MIC_PORT    I2S_NUM_0   // Port 0 for microphone
#define I2S_SPEAKER_PORT I2S_NUM_1  // Port 1 for speaker (separate)

// ======= Recorder State =======
enum RecorderState {
    RECORDER_IDLE,          // Not recording
    RECORDER_CONNECTING,    // Connecting to WebSocket
    RECORDER_RECORDING,     // Actively recording and streaming
    RECORDER_STOPPING,      // Finishing up
    RECORDER_ERROR          // Error state
};

// ======= MicRecorder Class =======
class MicRecorder {
public:
    // Singleton pattern
    static MicRecorder& getInstance() {
        static MicRecorder instance;
        return instance;
    }
    
    // Initialization
    bool begin();
    
    // Recording control
    bool startRecording(const String& serverUrl, const String& clientId);
    void stopRecording();
    
    // Must be called frequently (from task or loop)
    void update();
    
    // State
    bool isRecording();
    RecorderState getState();
    
    // Button handling (call from ISR or polling)
    void onButtonPressed();
    void onButtonReleased();
    
    // Configuration
    void setWebSocketUrl(const String& url);
    String getWebSocketUrl();
    
    // Debug
    void printStatus();

private:
    MicRecorder();
    ~MicRecorder();
    
    // Prevent copying
    MicRecorder(const MicRecorder&) = delete;
    MicRecorder& operator=(const MicRecorder&) = delete;
    
    // I2S
    bool initI2S();
    void deinitI2S();
    
    // WebSocket
    bool connectWebSocket(const String& url);
    void disconnectWebSocket();
    void sendAudioChunk(uint8_t* data, size_t len);
    
    // WebSocket event handler
    static void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    
    // State
    RecorderState state;
    volatile bool buttonPressed;
    bool wsConnected;
    
    // Audio buffer
    uint8_t* audioBuffer;
    size_t bufferIndex;
    
    // Configuration
    String wsServerUrl;
    String clientId;
    
    // WebSocket client
    WebSocketsClient webSocket;
    
    // Timing
    unsigned long lastSendTime;
    unsigned long recordStartTime;
    
    // Statistics
    uint32_t chunksRecorded;
    uint32_t chunksSent;
};

// Global pointer for ISR access
extern MicRecorder* micRecorderInstance;
extern volatile bool isDeviceRecording;  // Global flag for OTA/Audio blocking

#endif // MIC_RECORDER_H
