"""
============================================================================
OTA SERVER EXAMPLE - Python + Flask
============================================================================

Simple OTA server implementation for ESP32 OTA updates

Features:
- Check for firmware updates
- Download firmware binary
- Version management
- Device tracking

Setup:
1. pip install flask
2. python OTA_SERVER_EXAMPLE.py

============================================================================
"""

from flask import Flask, request, jsonify, send_file, send_from_directory
import os
from datetime import datetime
from pathlib import Path

app = Flask(__name__)

# ============================================================================
# CONFIGURATION
# ============================================================================

FIRMWARE_CONFIG = {
    # Danh sách các firmware versions
    'versions': {
        '1.0.0': {
            'filename': 'firmware_v1.0.0.bin',
            'changelog': 'Initial release',
            'release_date': '2024-01-01'
        },
        '1.0.1': {
            'filename': 'firmware_v1.0.1.bin',
            'changelog': 'Bug fixes and performance improvements',
            'release_date': '2024-01-15'
        },
        '1.1.0': {
            'filename': 'firmware_v1.1.0.bin',
            'changelog': 'New features: OTA update, MQTT improvements',
            'release_date': '2024-02-01'
        }
    },
    
    # Latest version
    'latest_version': '1.1.0',
    
    # Firmware directory
    'firmware_dir': os.path.join(os.path.dirname(__file__), 'firmware')
}

# Device tracking (optional)
device_log = {}

# ============================================================================
# HELPER FUNCTIONS
# ============================================================================

def compare_versions(v1, v2):
    """
    Compare two version strings
    Returns: -1 if v1 < v2, 0 if v1 === v2, 1 if v1 > v2
    """
    parts1 = [int(x) for x in v1.split('.')]
    parts2 = [int(x) for x in v2.split('.')]
    
    # Pad with zeros if lengths differ
    max_len = max(len(parts1), len(parts2))
    parts1 += [0] * (max_len - len(parts1))
    parts2 += [0] * (max_len - len(parts2))
    
    for p1, p2 in zip(parts1, parts2):
        if p1 < p2:
            return -1
        if p1 > p2:
            return 1
    
    return 0

def get_file_size(filename):
    """Get file size in bytes"""
    file_path = os.path.join(FIRMWARE_CONFIG['firmware_dir'], filename)
    
    try:
        return os.path.getsize(file_path)
    except:
        return 0

def format_size(size):
    """Format file size to human readable"""
    for unit in ['B', 'KB', 'MB', 'GB']:
        if size < 1024.0:
            return f"{size:.2f} {unit}"
        size /= 1024.0
    return f"{size:.2f} TB"

# ============================================================================
# MIDDLEWARE
# ============================================================================

@app.before_request
def log_request():
    """Log all incoming requests"""
    timestamp = datetime.now().isoformat()
    print(f"[{timestamp}] {request.method} {request.path}")
    if request.args:
        print(f"Query: {dict(request.args)}")

# ============================================================================
# ROUTES
# ============================================================================

@app.route('/')
def index():
    """Root endpoint"""
    return jsonify({
        'status': 'ok',
        'message': 'OTA Server is running',
        'version': FIRMWARE_CONFIG['latest_version'],
        'endpoints': {
            'check': '/ota/check?device_id={ID}&current_version={VERSION}',
            'download': '/ota/download/{FILENAME}'
        }
    })

@app.route('/ota/check')
def check_update():
    """
    Check for firmware updates
    GET /ota/check?device_id={ID}&current_version={VERSION}
    """
    device_id = request.args.get('device_id')
    current_version = request.args.get('current_version')
    
    print(f"\n📱 Check request from device: {device_id}")
    print(f"   Current version: {current_version}")
    
    # Validate parameters
    if not device_id or not current_version:
        return jsonify({
            'error': 'Missing required parameters: device_id, current_version'
        }), 400
    
    # Log device info
    device_log[device_id] = {
        'last_check': datetime.now().isoformat(),
        'current_version': current_version,
        'last_update': device_log.get(device_id, {}).get('last_update', 'Never')
    }
    
    # Compare versions
    latest_version = FIRMWARE_CONFIG['latest_version']
    has_update = compare_versions(current_version, latest_version) < 0
    
    if has_update:
        latest_firmware = FIRMWARE_CONFIG['versions'][latest_version]
        download_url = f"http://{request.host}/ota/download/{latest_firmware['filename']}"
        
        print(f"   ✅ Update available: {latest_version}")
        
        return jsonify({
            'has_update': True,
            'version': latest_version,
            'download_url': download_url,
            'changelog': latest_firmware['changelog'],
            'release_date': latest_firmware['release_date'],
            'file_size': get_file_size(latest_firmware['filename'])
        })
    else:
        print(f"   ℹ️  Already on latest version")
        
        return jsonify({
            'has_update': False,
            'version': current_version,
            'message': 'Already on latest version'
        })

@app.route('/ota/download/<filename>')
def download_firmware(filename):
    """
    Download firmware binary
    GET /ota/download/{filename}
    """
    file_path = os.path.join(FIRMWARE_CONFIG['firmware_dir'], filename)
    
    print(f"\n📥 Download request: {filename}")
    
    # Check if file exists
    if not os.path.exists(file_path):
        print(f"   ❌ File not found")
        return jsonify({
            'error': 'Firmware file not found'
        }), 404
    
    # Get file stats
    file_size = os.path.getsize(file_path)
    
    print(f"   File size: {file_size} bytes ({format_size(file_size)})")
    print(f"   Starting download...")
    
    try:
        # Send file
        return send_file(
            file_path,
            mimetype='application/octet-stream',
            as_attachment=True,
            download_name=filename
        )
    except Exception as e:
        print(f"   ❌ Error: {e}")
        return jsonify({
            'error': 'Error reading firmware file'
        }), 500

@app.route('/ota/devices')
def get_devices():
    """
    Get device logs
    GET /ota/devices
    """
    return jsonify({
        'total_devices': len(device_log),
        'devices': device_log
    })

@app.route('/ota/versions')
def get_versions():
    """
    Get available firmware versions
    GET /ota/versions
    """
    versions = []
    
    for version, info in FIRMWARE_CONFIG['versions'].items():
        versions.append({
            'version': version,
            'filename': info['filename'],
            'changelog': info['changelog'],
            'release_date': info['release_date'],
            'file_size': get_file_size(info['filename']),
            'file_size_formatted': format_size(get_file_size(info['filename'])),
            'is_latest': version == FIRMWARE_CONFIG['latest_version']
        })
    
    return jsonify({
        'latest_version': FIRMWARE_CONFIG['latest_version'],
        'total_versions': len(versions),
        'versions': versions
    })

# ============================================================================
# SERVER STARTUP
# ============================================================================

def main():
    # Create firmware directory if not exists
    firmware_dir = FIRMWARE_CONFIG['firmware_dir']
    if not os.path.exists(firmware_dir):
        os.makedirs(firmware_dir)
        print(f"📁 Created firmware directory: {firmware_dir}")
    
    # Print startup banner
    print('\n╔════════════════════════════════════════╗')
    print('║       ESP32 OTA SERVER RUNNING         ║')
    print('╠════════════════════════════════════════╣')
    print(f'║ Port:            5000                   ║')
    print(f'║ Latest Version:  {FIRMWARE_CONFIG["latest_version"]}                   ║')
    print(f'║ Firmware Dir:    {os.path.basename(firmware_dir)}            ║')
    print('╠════════════════════════════════════════╣')
    print('║ Endpoints:                             ║')
    print('║ GET /                                  ║')
    print('║ GET /ota/check                         ║')
    print('║ GET /ota/download/:filename            ║')
    print('║ GET /ota/devices                       ║')
    print('║ GET /ota/versions                      ║')
    print('╚════════════════════════════════════════╝\n')
    
    print('🌍 Server URL: http://localhost:5000')
    print('📡 OTA Check URL: http://localhost:5000/ota/check')
    print('\n✅ Ready to serve OTA updates!\n')
    
    # Start server
    app.run(host='0.0.0.0', port=5000, debug=False)

if __name__ == '__main__':
    main()

# ============================================================================
# USAGE INSTRUCTIONS
# ============================================================================

"""

📝 HOW TO USE THIS SERVER:

1. SETUP:
   - Install Python 3.7+ (https://www.python.org/)
   - Run: pip install flask
   - Create 'firmware' folder in same directory
   - Copy your .bin files to 'firmware' folder

2. FILE NAMING:
   - firmware_v1.0.0.bin
   - firmware_v1.0.1.bin
   - etc.

3. START SERVER:
   - Run: python OTA_SERVER_EXAMPLE.py
   - Server will start on http://localhost:5000

4. TEST WITH CURL:
   
   # Check for updates
   curl "http://localhost:5000/ota/check?device_id=test123&current_version=1.0.0"
   
   # Download firmware
   curl -O "http://localhost:5000/ota/download/firmware_v1.0.1.bin"
   
   # Get device logs
   curl "http://localhost:5000/ota/devices"
   
   # Get versions
   curl "http://localhost:5000/ota/versions"

5. HOW TO CREATE .BIN FILE:
   - In Arduino IDE, compile your sketch
   - After compilation, click "Sketch" → "Export compiled Binary"
   - The .bin file will be in your sketch folder
   - Copy it to the 'firmware' folder and rename according to version

6. ESP32 CONFIGURATION:
   - Update OTA_SERVER_URL in your ESP32 code
   - Change "localhost" to your computer's IP address
   - Example: "http://192.168.1.100:5000/ota"

7. FINDING YOUR IP ADDRESS:
   - Windows: ipconfig
   - Mac/Linux: ifconfig
   - Look for IPv4 Address (usually 192.168.x.x)

8. PYTHON-SPECIFIC NOTES:
   - Flask runs on port 5000 by default (Node.js example uses 8080)
   - You can change the port in the app.run() call
   - Set debug=True for development (auto-reload on code changes)
   - For production, use a WSGI server like Gunicorn

9. ADVANCED FEATURES:
   - Add authentication: Use Flask-HTTPAuth
   - Add HTTPS: Use SSL certificates with app.run(ssl_context=...)
   - Add logging: Use Python's logging module
   - Add database: Use SQLite or PostgreSQL for device tracking

"""

# ============================================================================
# EXAMPLE: Test with Python requests library
# ============================================================================

"""
import requests

# Test check endpoint
response = requests.get('http://localhost:5000/ota/check', params={
    'device_id': 'test123',
    'current_version': '1.0.0'
})
print(response.json())

# Test download endpoint
response = requests.get('http://localhost:5000/ota/download/firmware_v1.0.1.bin')
with open('downloaded_firmware.bin', 'wb') as f:
    f.write(response.content)
"""

