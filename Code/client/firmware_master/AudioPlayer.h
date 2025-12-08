#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h>
#include <driver/i2s.h>
#include <HTTPClient.h>
#include <WiFi.h>

// ======= I2S Configuration for MAX98357A =======
#ifndef SPEAKER_I2S_BCLK
#define SPEAKER_I2S_BCLK  26    // Bit Clock
#endif
#ifndef SPEAKER_I2S_LRC
#define SPEAKER_I2S_LRC   25    // Word Select (LRCLK)
#endif
#ifndef SPEAKER_I2S_DOUT
#define SPEAKER_I2S_DOUT  22    // Data Out
#endif

// I2S port for speaker (separate from microphone)
#define I2S_SPEAKER_PORT  I2S_NUM_1

// Audio playback parameters
#define AUDIO_SAMPLE_RATE    16000   // Match STT output
#define AUDIO_BITS           16
#define AUDIO_CHANNELS       1       // Mono
#define AUDIO_BUFFER_SIZE    4096    // Playback buffer

// Max reconnect attempts
#define MAX_RECONNECT_ATTEMPTS 3

// ======= Player State =======
enum PlayerState {
    PLAYER_IDLE,
    PLAYER_PLAYING,
    PLAYER_STOPPED,
    PLAYER_PAUSED,
    PLAYER_ERROR
};

// ======= WAV Header Structure =======
struct WavHeader {
    char riff[4];           // "RIFF"
    uint32_t fileSize;      // File size - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmtSize;       // Format chunk size (16 for PCM)
    uint16_t audioFormat;   // 1 = PCM
    uint16_t numChannels;   // 1 = Mono, 2 = Stereo
    uint32_t sampleRate;    // Sample rate
    uint32_t byteRate;      // Bytes per second
    uint16_t blockAlign;    // Bytes per sample * channels
    uint16_t bitsPerSample; // Bits per sample
    char data[4];           // "data"
    uint32_t dataSize;      // Data chunk size
};

// ======= AudioPlayer Class =======
class AudioPlayer {
public:
    // Singleton pattern
    static AudioPlayer& getInstance() {
        static AudioPlayer instance;
        return instance;
    }
    
    // Initialization
    bool begin(int bclkPin = SPEAKER_I2S_BCLK, 
               int lrcPin = SPEAKER_I2S_LRC, 
               int doutPin = SPEAKER_I2S_DOUT);
    void end();
    
    // Playback control
    bool play(const String& url);
    bool play(const char* url);
    void stop();
    void pause();
    void resume();
    
    // Must be called frequently (from task or loop)
    void update();
    
    // Status
    bool isPlaying();
    bool isPaused();
    PlayerState getState();
    String getCurrentUrl();
    
    // Volume control (0.0 - 1.0)
    void setVolume(float volume);
    float getVolume();
    
    // Settings
    void setAutoReconnect(bool enable);
    
    // Debug
    void printStatus();

private:
    AudioPlayer();
    ~AudioPlayer();
    
    // Prevent copying
    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;
    
    // I2S
    bool initI2S();
    void deinitI2S();
    bool writeI2S(uint8_t* data, size_t len);
    
    // HTTP streaming
    bool openStream(const String& url);
    void closeStream();
    bool readWavHeader();
    int readAudioData(uint8_t* buffer, size_t len);
    
    // State
    PlayerState state;
    bool i2sInitialized;
    bool autoReconnect;
    
    // Audio buffer
    uint8_t* audioBuffer;
    
    // Configuration
    String currentUrl;
    float currentVolume;
    int pinBCLK;
    int pinLRC;
    int pinDOUT;
    
    // HTTP client
    HTTPClient http;
    WiFiClient* stream;
    
    // WAV info
    WavHeader wavHeader;
    uint32_t dataRemaining;
    
    // Statistics
    uint32_t bytesPlayed;
    int reconnectAttempts;
};

// Global flag to check if audio is playing
extern volatile bool isAudioPlaying;

#endif // AUDIO_PLAYER_H
