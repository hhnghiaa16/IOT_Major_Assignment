#include "AudioPlayer.h"

// Global flag
volatile bool isAudioPlaying = false;

// ======= Constructor & Destructor =======
AudioPlayer::AudioPlayer() {
    state = PLAYER_IDLE;
    i2sInitialized = false;
    autoReconnect = true;
    audioBuffer = nullptr;
    currentVolume = 1.0;
    pinBCLK = SPEAKER_I2S_BCLK;
    pinLRC = SPEAKER_I2S_LRC;
    pinDOUT = SPEAKER_I2S_DOUT;
    stream = nullptr;
    dataRemaining = 0;
    bytesPlayed = 0;
    reconnectAttempts = 0;
}

AudioPlayer::~AudioPlayer() {
    end();
}

// ======= Initialization =======
bool AudioPlayer::begin(int bclkPin, int lrcPin, int doutPin) {
    Serial.println("[AudioPlayer] Initializing...");
    
    pinBCLK = bclkPin;
    pinLRC = lrcPin;
    pinDOUT = doutPin;
    
    // Allocate audio buffer
    audioBuffer = (uint8_t*)malloc(AUDIO_BUFFER_SIZE);
    if (!audioBuffer) {
        Serial.println("[AudioPlayer] ERROR: Failed to allocate buffer!");
        state = PLAYER_ERROR;
        return false;
    }
    
    Serial.printf("[AudioPlayer] Initialized successfully\n");
    Serial.printf("[AudioPlayer] I2S Pins - BCLK:%d, LRC:%d, DOUT:%d\n", 
                  pinBCLK, pinLRC, pinDOUT);
    
    state = PLAYER_IDLE;
    return true;
}

void AudioPlayer::end() {
    stop();
    if (audioBuffer) {
        free(audioBuffer);
        audioBuffer = nullptr;
    }
    deinitI2S();
}

// ======= I2S Functions =======
bool AudioPlayer::initI2S() {
    if (i2sInitialized) return true;
    
    Serial.println("[AudioPlayer] Configuring I2S for MAX98357A...");
    
    // I2S configuration for speaker output
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = wavHeader.sampleRate > 0 ? wavHeader.sampleRate : AUDIO_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // Mono output
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    
    // I2S pin configuration for speaker
    i2s_pin_config_t pin_config = {
        .bck_io_num = pinBCLK,
        .ws_io_num = pinLRC,
        .data_out_num = pinDOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    
    // Install I2S driver
    esp_err_t err = i2s_driver_install(I2S_SPEAKER_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[AudioPlayer] ERROR: I2S driver install failed: %d\n", err);
        return false;
    }
    
    err = i2s_set_pin(I2S_SPEAKER_PORT, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[AudioPlayer] ERROR: I2S set pin failed: %d\n", err);
        i2s_driver_uninstall(I2S_SPEAKER_PORT);
        return false;
    }
    
    // Clear DMA buffer
    i2s_zero_dma_buffer(I2S_SPEAKER_PORT);
    
    i2sInitialized = true;
    Serial.println("[AudioPlayer] I2S initialized for speaker");
    return true;
}

void AudioPlayer::deinitI2S() {
    if (i2sInitialized) {
        i2s_driver_uninstall(I2S_SPEAKER_PORT);
        i2sInitialized = false;
        Serial.println("[AudioPlayer] I2S deinitialized");
    }
}

bool AudioPlayer::writeI2S(uint8_t* data, size_t len) {
    if (!i2sInitialized || len == 0) return false;
    
    // Apply volume
    if (currentVolume < 1.0) {
        int16_t* samples = (int16_t*)data;
        size_t numSamples = len / 2;
        for (size_t i = 0; i < numSamples; i++) {
            samples[i] = (int16_t)(samples[i] * currentVolume);
        }
    }
    
    size_t bytesWritten = 0;
    esp_err_t err = i2s_write(I2S_SPEAKER_PORT, data, len, &bytesWritten, pdMS_TO_TICKS(100));
    
    if (err != ESP_OK) {
        Serial.printf("[AudioPlayer] I2S write error: %d\n", err);
        return false;
    }
    
    bytesPlayed += bytesWritten;
    return true;
}

// ======= HTTP Streaming =======
bool AudioPlayer::openStream(const String& url) {
    Serial.printf("[AudioPlayer] Opening stream: %s\n", url.c_str());
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[AudioPlayer] ERROR: WiFi not connected!");
        return false;
    }
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[AudioPlayer] HTTP error: %d\n", httpCode);
        http.end();
        return false;
    }
    
    stream = http.getStreamPtr();
    if (!stream) {
        Serial.println("[AudioPlayer] ERROR: Failed to get stream!");
        http.end();
        return false;
    }
    
    Serial.printf("[AudioPlayer] Stream opened, size: %d bytes\n", http.getSize());
    return true;
}

void AudioPlayer::closeStream() {
    http.end();
    stream = nullptr;
    Serial.println("[AudioPlayer] Stream closed");
}

bool AudioPlayer::readWavHeader() {
    if (!stream) return false;
    
    // Read WAV header (44 bytes standard)
    uint8_t header[44];
    size_t bytesRead = stream->readBytes(header, 44);
    
    if (bytesRead < 44) {
        Serial.println("[AudioPlayer] ERROR: Failed to read WAV header!");
        return false;
    }
    
    // Parse header
    memcpy(&wavHeader, header, sizeof(WavHeader));
    
    // Validate
    if (strncmp(wavHeader.riff, "RIFF", 4) != 0 || 
        strncmp(wavHeader.wave, "WAVE", 4) != 0) {
        Serial.println("[AudioPlayer] ERROR: Invalid WAV format!");
        return false;
    }
    
    Serial.printf("[AudioPlayer] WAV Info:\n");
    Serial.printf("  Sample Rate: %u Hz\n", wavHeader.sampleRate);
    Serial.printf("  Bits/Sample: %u\n", wavHeader.bitsPerSample);
    Serial.printf("  Channels: %u\n", wavHeader.numChannels);
    Serial.printf("  Data Size: %u bytes\n", wavHeader.dataSize);
    
    dataRemaining = wavHeader.dataSize;
    return true;
}

int AudioPlayer::readAudioData(uint8_t* buffer, size_t len) {
    if (!stream || dataRemaining == 0) return 0;
    
    size_t toRead = min(len, (size_t)dataRemaining);
    size_t bytesRead = stream->readBytes(buffer, toRead);
    
    dataRemaining -= bytesRead;
    return bytesRead;
}

// ======= Playback Control =======
bool AudioPlayer::play(const String& url) {
    return play(url.c_str());
}

bool AudioPlayer::play(const char* url) {
    if (state == PLAYER_PLAYING) {
        stop();
        delay(100);
    }
    
    Serial.printf("[AudioPlayer] Playing: %s\n", url);
    
    currentUrl = String(url);
    reconnectAttempts = 0;
    bytesPlayed = 0;
    
    // Open HTTP stream
    if (!openStream(currentUrl)) {
        state = PLAYER_ERROR;
        return false;
    }
    
    // Read and parse WAV header
    if (!readWavHeader()) {
        closeStream();
        state = PLAYER_ERROR;
        return false;
    }
    
    // Initialize I2S with correct sample rate
    deinitI2S();  // Reinit with new sample rate if needed
    if (!initI2S()) {
        closeStream();
        state = PLAYER_ERROR;
        return false;
    }
    
    state = PLAYER_PLAYING;
    isAudioPlaying = true;
    
    Serial.println("[AudioPlayer] ✓ Playback started!");
    return true;
}

void AudioPlayer::stop() {
    if (state == PLAYER_IDLE) return;
    
    Serial.println("[AudioPlayer] Stopping...");
    
    closeStream();
    deinitI2S();
    
    state = PLAYER_STOPPED;
    isAudioPlaying = false;
    
    Serial.printf("[AudioPlayer] ✓ Stopped. Bytes played: %u\n", bytesPlayed);
}

void AudioPlayer::pause() {
    if (state == PLAYER_PLAYING) {
        state = PLAYER_PAUSED;
        isAudioPlaying = false;
        Serial.println("[AudioPlayer] Paused");
    }
}

void AudioPlayer::resume() {
    if (state == PLAYER_PAUSED) {
        state = PLAYER_PLAYING;
        isAudioPlaying = true;
        Serial.println("[AudioPlayer] Resumed");
    }
}

// ======= Update (call in loop/task) =======
void AudioPlayer::update() {
    if (state != PLAYER_PLAYING || !stream || !audioBuffer) {
        return;
    }
    
    // Read audio data from stream
    int bytesRead = readAudioData(audioBuffer, AUDIO_BUFFER_SIZE);
    
    if (bytesRead > 0) {
        // Write to I2S speaker
        writeI2S(audioBuffer, bytesRead);
    }
    
    // Check if playback finished
    if (dataRemaining == 0 || bytesRead == 0) {
        // Kiểm tra phát xong bình thường hay bị lỗi
        bool isNormalCompletion = (dataRemaining == 0);
        
        if (isNormalCompletion) {
            // Phát xong bình thường → dừng
            Serial.println("[AudioPlayer] Stream ended - playback complete");
            stop();
        } else {
            // Stream bị lỗi giữa chừng → retry
            Serial.printf("[AudioPlayer] Stream error! Data remaining: %u bytes\n", dataRemaining);
            
            if (autoReconnect && reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
                reconnectAttempts++;
                Serial.printf("[AudioPlayer] Auto-reconnecting... (attempt %d/%d)\n", 
                              reconnectAttempts, MAX_RECONNECT_ATTEMPTS);
                closeStream();
                delay(1000);
                
                if (!play(currentUrl.c_str())) {
                    state = PLAYER_ERROR;
                    isAudioPlaying = false;
                }
            } else {
                Serial.println("[AudioPlayer] Max reconnect attempts reached, stopping");
                stop();
            }
        }
    }
}

// ======= Status =======
bool AudioPlayer::isPlaying() {
    return (state == PLAYER_PLAYING);
}

bool AudioPlayer::isPaused() {
    return (state == PLAYER_PAUSED);
}

PlayerState AudioPlayer::getState() {
    return state;
}

String AudioPlayer::getCurrentUrl() {
    return currentUrl;
}

// ======= Volume Control =======
void AudioPlayer::setVolume(float volume) {
    if (volume < 0.0) volume = 0.0;
    if (volume > 1.0) volume = 1.0;
    currentVolume = volume;
    Serial.printf("[AudioPlayer] Volume set to: %.2f\n", currentVolume);
}

float AudioPlayer::getVolume() {
    return currentVolume;
}

// ======= Settings =======
void AudioPlayer::setAutoReconnect(bool enable) {
    autoReconnect = enable;
    Serial.printf("[AudioPlayer] Auto-reconnect: %s\n", enable ? "ON" : "OFF");
}

// ======= Debug =======
void AudioPlayer::printStatus() {
    Serial.println("\n===== AudioPlayer Status =====");
    Serial.printf("State: ");
    switch(state) {
        case PLAYER_IDLE: Serial.println("IDLE"); break;
        case PLAYER_PLAYING: Serial.println("PLAYING"); break;
        case PLAYER_STOPPED: Serial.println("STOPPED"); break;
        case PLAYER_PAUSED: Serial.println("PAUSED"); break;
        case PLAYER_ERROR: Serial.println("ERROR"); break;
    }
    Serial.printf("URL: %s\n", currentUrl.c_str());
    Serial.printf("Volume: %.2f\n", currentVolume);
    Serial.printf("Auto-reconnect: %s\n", autoReconnect ? "ON" : "OFF");
    Serial.printf("I2S Pins - BCLK:%d, LRC:%d, DOUT:%d\n", pinBCLK, pinLRC, pinDOUT);
    Serial.printf("I2S Initialized: %s\n", i2sInitialized ? "YES" : "NO");
    Serial.printf("Bytes Played: %u\n", bytesPlayed);
    Serial.println("==============================\n");
}