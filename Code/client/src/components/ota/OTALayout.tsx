/**
 * OTA Layout Component
 * Layout with sidebar navigation for OTA update pages
 */

import React, { useState } from 'react';
import '../../styles/OTALayout.css';
import DeviceUpdatePage from './DeviceUpdatePage';
import FirmwareUploadPage from './FirmwareUploadPage';

type OTASubPage = 'device-update' | 'firmware-upload';

const OTALayout: React.FC = () => {
  const [activePage, setActivePage] = useState<OTASubPage>('device-update');

  return (
    <div className="ota-layout">
      <div className="ota-sidebar">
        <div className="ota-sidebar-header">
          <h2 className="ota-sidebar-title">Cập nhật OTA</h2>
        </div>
        <nav className="ota-sidebar-nav">
          <button
            onClick={() => setActivePage('device-update')}
            className={`ota-nav-item ${activePage === 'device-update' ? 'active' : ''}`}
          >
            <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M12 2L2 7v10c0 5.55 3.84 10.74 9 12 5.16-1.26 9-6.45 9-12V7l-10-5z" />
              <path d="M12 22v-8M8 12l4-4 4 4" />
            </svg>
            <span>Cập nhật thiết bị</span>
          </button>
          <button
            onClick={() => setActivePage('firmware-upload')}
            className={`ota-nav-item ${activePage === 'firmware-upload' ? 'active' : ''}`}
          >
            <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4M17 8l-5-5-5 5M12 3v12" />
            </svg>
            <span>Quản lý File & Upload</span>
          </button>
        </nav>
      </div>
      <div className="ota-main-content">
        {activePage === 'device-update' ? (
          <DeviceUpdatePage />
        ) : (
          <FirmwareUploadPage />
        )}
      </div>
    </div>
  );
};

export default OTALayout;

