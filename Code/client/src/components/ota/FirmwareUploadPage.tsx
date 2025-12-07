/**
 * Firmware Upload Page Component
 */

import React, { useState, useEffect, useRef } from 'react';
import '../../styles/FirmwareUploadPage.css';
import deviceService from '../../services/deviceService';
import otaService from '../../services/otaService';
import type { DeviceWithOTA } from '../../types';

interface FirmwareFormData {
  filename: string;
  change_log: string;
  version: string;
  type: string;
}

const FirmwareUploadPage: React.FC = () => {
  const [devices, setDevices] = useState<DeviceWithOTA[]>([]);
  const [loading, setLoading] = useState(true);
  const [uploading, setUploading] = useState(false);
  const [file, setFile] = useState<File | null>(null);
  const [formData, setFormData] = useState<FirmwareFormData>({
    filename: '',
    change_log: '',
    version: '',
    type: '1'
  });
  const [dragActive, setDragActive] = useState(false);
  const fileInputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    fetchDevicesAndOTAInfo();
  }, []);

  const fetchDevicesAndOTAInfo = async () => {
    try {
      setLoading(true);
      const devicesList = await deviceService.getDevices();

      const devicesWithOTA = await Promise.all(
        devicesList.map(async (device) => {
          if (!device.token_verify) {
            return { ...device, otaInfo: undefined };
          }
          try {
            const otaInfo = await otaService.checkOTAInfo(device.token_verify, 0);
            // Normalize API response
            const normalizedOtaInfo = {
              is_updating: otaInfo.is_updating ?? false,
              on_progress: typeof otaInfo.on_progress === 'number' ? otaInfo.on_progress : 0,
              auto_update: otaInfo.auto_update ?? false,
              lastVersion: otaInfo.lastVersion ?? null,
              lastUpdate: otaInfo.lastUpdate ?? null,
              hasNewVersion: otaInfo.hasNewVersion ?? false,
              error: Array.isArray(otaInfo.error) ? otaInfo.error : [],
            };
            return { ...device, otaInfo: normalizedOtaInfo };
          } catch (err) {
            return { ...device, otaInfo: undefined };
          }
        })
      );

      setDevices(devicesWithOTA);
    } catch (err) {
      console.error('Failed to fetch devices:', err);
    } finally {
      setLoading(false);
    }
  };

  const handleDrag = (e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    if (e.type === "dragenter" || e.type === "dragover") {
      setDragActive(true);
    } else if (e.type === "dragleave") {
      setDragActive(false);
    }
  };

  const handleDrop = (e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    setDragActive(false);

    if (e.dataTransfer.files && e.dataTransfer.files[0]) {
      handleFileSelect(e.dataTransfer.files[0]);
    }
  };

  const handleChange = (e: React.ChangeEvent<HTMLInputElement | HTMLTextAreaElement | HTMLSelectElement>) => {
    const { name, value } = e.target;
    setFormData(prev => ({ ...prev, [name]: value }));
  };

  const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    if (e.target.files && e.target.files[0]) {
      handleFileSelect(e.target.files[0]);
    }
  };

  const handleFileSelect = (selectedFile: File) => {
    if (selectedFile.name.endsWith('.bin')) {
      setFile(selectedFile);
      // Auto-fill filename if empty
      if (!formData.filename) {
        setFormData(prev => ({ ...prev, filename: selectedFile.name }));
      }
    } else {
      alert('Please upload a .bin file');
    }
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!file) {
      alert('Please select a file');
      return;
    }

    try {
      setUploading(true);
      const data = new FormData();
      data.append('file', file);
      data.append('filename', formData.filename);
      data.append('change_log', formData.change_log);
      data.append('version', formData.version);
      data.append('type', formData.type);

      const response = await otaService.uploadFirmware(data);
      if (response.success) {
        alert('Firmware uploaded successfully!');
        // Reset form
        setFile(null);
        setFormData({
          filename: '',
          change_log: '',
          version: '',
          type: '1'
        });
        if (fileInputRef.current) fileInputRef.current.value = '';
      }
    } catch (err) {
      alert(err instanceof Error ? err.message : 'Upload failed');
    } finally {
      setUploading(false);
    }
  };

  const formatDate = (dateString: string | null | undefined): string => {
    if (!dateString) return 'Never updated';
    try {
      const date = new Date(dateString);
      // Calculate days ago
      const diffTime = Math.abs(new Date().getTime() - date.getTime());
      const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24));

      if (diffDays === 1) return '1 day ago';
      if (diffDays < 30) return `${diffDays} days ago`;

      return date.toLocaleDateString();
    } catch {
      return 'Invalid date';
    }
  };

  return (
    <div className="firmware-upload-page">
      <div className="page-header">
        <div className="page-header-left">
          <h1 className="page-title">Firmware Management</h1>
          <p className="page-description">Upload new firmware versions and track release history</p>
        </div>
        <button onClick={fetchDevicesAndOTAInfo} className="btn-refresh">
          <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
            <path d="M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8M21 8v5M21 8h-5M3 12a9 9 0 0 0 9 9 9.75 9.75 0 0 0 6.74-2.74L3 16M3 16v-5M3 16h5" />
          </svg>
          Refresh
        </button>
      </div>

      <div className="firmware-main-content">
        {/* Left Side: Device Summary */}
        <div className="section-card">
          <h3 className="section-title">Device Summary</h3>
          <p className="section-description">Quick overview of connected devices</p>

          <div className="device-list">
            {loading ? (
              <div className="loading-container">
                <div className="spinner-small"></div>
                <p>Loading devices...</p>
              </div>
            ) : devices.length === 0 ? (
              <div className="empty-container">No devices found</div>
            ) : (
              devices.map(device => (
                <div className="device-item" key={device.id}>
                  <div className="device-info">
                    <div className="device-icon">
                      <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                        <rect x="2" y="3" width="20" height="14" rx="2" ry="2" />
                        <line x1="8" y1="21" x2="16" y2="21" />
                        <line x1="12" y1="17" x2="12" y2="21" />
                      </svg>
                    </div>
                    <div className="device-details">
                      <h4>{device.device_name}</h4>
                      <p className="device-version">v{device.otaInfo?.lastVersion || '?.?.?'}</p>
                    </div>
                  </div>
                  <div className="device-time">
                    <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                      <circle cx="12" cy="12" r="10" />
                      <polyline points="12 6 12 12 16 14" />
                    </svg>
                    <span>{formatDate(device.otaInfo?.lastUpdate)}</span>
                  </div>
                </div>
              ))
            )}
          </div>
        </div>

        {/* Right Side: Upload Form */}
        <div className="section-card">
          <h3 className="section-title">Upload New Firmware</h3>
          <p className="section-description">Upload a .bin file to deploy to your devices</p>

          <form className="upload-form" onSubmit={handleSubmit}>
            <div
              className={`file-drop-zone ${dragActive ? 'active' : ''}`}
              onDragEnter={handleDrag}
              onDragLeave={handleDrag}
              onDragOver={handleDrag}
              onDrop={handleDrop}
              onClick={() => fileInputRef.current?.click()}
            >
              <input
                ref={fileInputRef}
                type="file"
                accept=".bin"
                style={{ display: 'none' }}
                onChange={handleFileChange}
              />

              {file ? (
                <>
                  <div className="upload-icon">
                    <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                      <path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z" />
                      <polyline points="14 2 14 8 20 8" />
                      <line x1="12" y1="18" x2="12" y2="12" />
                      <line x1="9" y1="15" x2="15" y2="15" />
                    </svg>
                  </div>
                  <p className="upload-text">{file.name}</p>
                  <p className="upload-subtext">{(file.size / 1024).toFixed(2)} KB</p>
                </>
              ) : (
                <>
                  <div className="upload-icon">
                    <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                      <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4" />
                      <polyline points="17 8 12 3 7 8" />
                      <line x1="12" y1="3" x2="12" y2="15" />
                    </svg>
                  </div>
                  <p className="upload-text">Drag and drop .bin firmware file here</p>
                  <p className="upload-subtext">or click to browse files</p>
                </>
              )}
            </div>

            <div className="form-group">
              <label className="form-label">Filename</label>
              <input
                type="text"
                name="filename"
                className="form-input"
                value={formData.filename}
                onChange={handleChange}
                placeholder="e.g. firmware_v2.bin"
                required
              />
            </div>

            <div className="form-group">
              <label className="form-label">Version</label>
              <input
                type="text"
                name="version"
                className="form-input"
                value={formData.version}
                onChange={handleChange}
                placeholder="e.g. 2.4.1"
                required
              />
            </div>

            <div className="form-group">
              <label className="form-label">Type (1 : slave, 0 : master)</label>
              <input
                type="text"
                name="type"
                className="form-input"
                value={formData.type}
                onChange={handleChange}
                placeholder="e.g. ESP32"
                required
              />
            </div>

            <div className="form-group">
              <label className="form-label">Changelog</label>
              <textarea
                name="change_log"
                className="form-textarea"
                value={formData.change_log}
                onChange={handleChange}
                placeholder="Describe what's new in this version..."
                required
              />
            </div>

            <button type="submit" className="btn-upload" disabled={uploading}>
              {uploading ? (
                <>
                  <div className="loading-spinner"></div>
                  Uploading...
                </>
              ) : (
                <>
                  <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                    <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4" />
                    <polyline points="17 8 12 3 7 8" />
                    <line x1="12" y1="3" x2="12" y2="15" />
                  </svg>
                  Upload Firmware
                </>
              )}
            </button>
          </form>
        </div>
      </div>
    </div>
  );
};

export default FirmwareUploadPage;
