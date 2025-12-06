/**
 * ============================================================================
 * OTA SERVER EXAMPLE - Node.js + Express
 * ============================================================================
 * 
 * Simple OTA server implementation for ESP32 OTA updates
 * 
 * Features:
 * - Check for firmware updates
 * - Download firmware binary
 * - Version management
 * - Device tracking
 * 
 * Setup:
 * 1. npm install express
 * 2. node OTA_SERVER_EXAMPLE.js
 * 
 * ============================================================================
 */

const express = require('express');
const fs = require('fs');
const path = require('path');

const app = express();
const PORT = 8080;

// ============================================================================
// CONFIGURATION
// ============================================================================

const FIRMWARE_CONFIG = {
    // Danh sách các firmware versions
    versions: {
        '1.0.0': {
            filename: 'firmware_v1.0.0.bin',
            changelog: 'Initial release',
            releaseDate: '2024-01-01'
        },
        '1.0.1': {
            filename: 'firmware_v1.0.1.bin',
            changelog: 'Bug fixes and performance improvements',
            releaseDate: '2024-01-15'
        },
        '1.1.0': {
            filename: 'firmware_v1.1.0.bin',
            changelog: 'New features: OTA update, MQTT improvements',
            releaseDate: '2024-02-01'
        }
    },
    
    // Latest version
    latestVersion: '1.1.0',
    
    // Firmware directory
    firmwareDir: path.join(__dirname, 'firmware')
};

// Device tracking (optional)
const deviceLog = {};

// ============================================================================
// MIDDLEWARE
// ============================================================================

// Enable JSON parsing
app.use(express.json());

// Logging middleware
app.use((req, res, next) => {
    const timestamp = new Date().toISOString();
    console.log(`[${timestamp}] ${req.method} ${req.path}`);
    console.log('Query:', req.query);
    next();
});

// ============================================================================
// ROUTES
// ============================================================================

/**
 * Root endpoint
 */
app.get('/', (req, res) => {
    res.json({
        status: 'ok',
        message: 'OTA Server is running',
        version: FIRMWARE_CONFIG.latestVersion,
        endpoints: {
            check: '/ota/check?device_id={ID}&current_version={VERSION}',
            download: '/ota/download/{FILENAME}'
        }
    });
});

/**
 * Check for firmware updates
 * GET /ota/check?device_id={ID}&current_version={VERSION}
 */
app.get('/ota/check', (req, res) => {
    const { device_id, current_version } = req.query;
    
    console.log(`\n📱 Check request from device: ${device_id}`);
    console.log(`   Current version: ${current_version}`);
    
    // Validate parameters
    if (!device_id || !current_version) {
        return res.status(400).json({
            error: 'Missing required parameters: device_id, current_version'
        });
    }
    
    // Log device info
    deviceLog[device_id] = {
        lastCheck: new Date().toISOString(),
        currentVersion: current_version,
        lastUpdate: deviceLog[device_id]?.lastUpdate || 'Never'
    };
    
    // Compare versions
    const latestVersion = FIRMWARE_CONFIG.latestVersion;
    const hasUpdate = compareVersions(current_version, latestVersion) < 0;
    
    if (hasUpdate) {
        const latestFirmware = FIRMWARE_CONFIG.versions[latestVersion];
        const downloadUrl = `http://${req.hostname}:${PORT}/ota/download/${latestFirmware.filename}`;
        
        console.log(`   ✅ Update available: ${latestVersion}`);
        
        res.json({
            has_update: true,
            version: latestVersion,
            download_url: downloadUrl,
            changelog: latestFirmware.changelog,
            release_date: latestFirmware.releaseDate,
            file_size: getFileSize(latestFirmware.filename)
        });
    } else {
        console.log(`   ℹ️  Already on latest version`);
        
        res.json({
            has_update: false,
            version: current_version,
            message: 'Already on latest version'
        });
    }
});

/**
 * Download firmware binary
 * GET /ota/download/{filename}
 */
app.get('/ota/download/:filename', (req, res) => {
    const { filename } = req.params;
    const filePath = path.join(FIRMWARE_CONFIG.firmwareDir, filename);
    
    console.log(`\n📥 Download request: ${filename}`);
    
    // Check if file exists
    if (!fs.existsSync(filePath)) {
        console.log(`   ❌ File not found`);
        return res.status(404).json({
            error: 'Firmware file not found'
        });
    }
    
    // Get file stats
    const stats = fs.statSync(filePath);
    const fileSize = stats.size;
    
    console.log(`   File size: ${fileSize} bytes (${(fileSize / 1024).toFixed(2)} KB)`);
    console.log(`   Starting download...`);
    
    // Set headers
    res.setHeader('Content-Type', 'application/octet-stream');
    res.setHeader('Content-Length', fileSize);
    res.setHeader('Content-Disposition', `attachment; filename="${filename}"`);
    
    // Stream file to client
    const fileStream = fs.createReadStream(filePath);
    
    fileStream.on('error', (err) => {
        console.error(`   ❌ Error reading file:`, err);
        res.status(500).json({ error: 'Error reading firmware file' });
    });
    
    fileStream.on('end', () => {
        console.log(`   ✅ Download completed`);
    });
    
    fileStream.pipe(res);
});

/**
 * Get device logs
 * GET /ota/devices
 */
app.get('/ota/devices', (req, res) => {
    res.json({
        total_devices: Object.keys(deviceLog).length,
        devices: deviceLog
    });
});

/**
 * Get available firmware versions
 * GET /ota/versions
 */
app.get('/ota/versions', (req, res) => {
    const versions = Object.keys(FIRMWARE_CONFIG.versions).map(version => ({
        version: version,
        ...FIRMWARE_CONFIG.versions[version],
        file_size: getFileSize(FIRMWARE_CONFIG.versions[version].filename),
        is_latest: version === FIRMWARE_CONFIG.latestVersion
    }));
    
    res.json({
        latest_version: FIRMWARE_CONFIG.latestVersion,
        total_versions: versions.length,
        versions: versions
    });
});

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Compare two version strings
 * Returns: -1 if v1 < v2, 0 if v1 === v2, 1 if v1 > v2
 */
function compareVersions(v1, v2) {
    const parts1 = v1.split('.').map(Number);
    const parts2 = v2.split('.').map(Number);
    
    for (let i = 0; i < Math.max(parts1.length, parts2.length); i++) {
        const p1 = parts1[i] || 0;
        const p2 = parts2[i] || 0;
        
        if (p1 < p2) return -1;
        if (p1 > p2) return 1;
    }
    
    return 0;
}

/**
 * Get file size in bytes
 */
function getFileSize(filename) {
    const filePath = path.join(FIRMWARE_CONFIG.firmwareDir, filename);
    
    try {
        const stats = fs.statSync(filePath);
        return stats.size;
    } catch (err) {
        return 0;
    }
}

// ============================================================================
// SERVER STARTUP
// ============================================================================

// Create firmware directory if not exists
if (!fs.existsSync(FIRMWARE_CONFIG.firmwareDir)) {
    fs.mkdirSync(FIRMWARE_CONFIG.firmwareDir, { recursive: true });
    console.log(`📁 Created firmware directory: ${FIRMWARE_CONFIG.firmwareDir}`);
}

// Start server
app.listen(PORT, () => {
    console.log('\n╔════════════════════════════════════════╗');
    console.log('║       ESP32 OTA SERVER RUNNING         ║');
    console.log('╠════════════════════════════════════════╣');
    console.log(`║ Port:            ${PORT}                    ║`);
    console.log(`║ Latest Version:  ${FIRMWARE_CONFIG.latestVersion}                   ║`);
    console.log(`║ Firmware Dir:    ${path.basename(FIRMWARE_CONFIG.firmwareDir)}            ║`);
    console.log('╠════════════════════════════════════════╣');
    console.log('║ Endpoints:                             ║');
    console.log('║ GET /                                  ║');
    console.log('║ GET /ota/check                         ║');
    console.log('║ GET /ota/download/:filename            ║');
    console.log('║ GET /ota/devices                       ║');
    console.log('║ GET /ota/versions                      ║');
    console.log('╚════════════════════════════════════════╝\n');
    
    console.log(`🌍 Server URL: http://localhost:${PORT}`);
    console.log(`📡 OTA Check URL: http://localhost:${PORT}/ota/check`);
    console.log('\n✅ Ready to serve OTA updates!\n');
});

// ============================================================================
// USAGE INSTRUCTIONS
// ============================================================================

/*

📝 HOW TO USE THIS SERVER:

1. SETUP:
   - Install Node.js (https://nodejs.org/)
   - Run: npm install express
   - Create 'firmware' folder in same directory
   - Copy your .bin files to 'firmware' folder

2. FILE NAMING:
   - firmware_v1.0.0.bin
   - firmware_v1.0.1.bin
   - etc.

3. START SERVER:
   - Run: node OTA_SERVER_EXAMPLE.js
   - Server will start on http://localhost:8080

4. TEST WITH CURL:
   
   # Check for updates
   curl "http://localhost:8080/ota/check?device_id=test123&current_version=1.0.0"
   
   # Download firmware
   curl -O "http://localhost:8080/ota/download/firmware_v1.0.1.bin"
   
   # Get device logs
   curl "http://localhost:8080/ota/devices"
   
   # Get versions
   curl "http://localhost:8080/ota/versions"

5. HOW TO CREATE .BIN FILE:
   - In Arduino IDE, compile your sketch
   - After compilation, click "Sketch" → "Export compiled Binary"
   - The .bin file will be in your sketch folder
   - Copy it to the 'firmware' folder and rename according to version

6. ESP32 CONFIGURATION:
   - Update OTA_SERVER_URL in your ESP32 code
   - Change "localhost" to your computer's IP address
   - Example: "http://192.168.1.100:8080/ota"

7. FINDING YOUR IP ADDRESS:
   - Windows: ipconfig
   - Mac/Linux: ifconfig
   - Look for IPv4 Address (usually 192.168.x.x)

*/

