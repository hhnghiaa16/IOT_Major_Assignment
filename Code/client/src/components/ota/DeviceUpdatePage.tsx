/**
 * Device Update Page Component
 * Page for managing device OTA updates
 */

import React from 'react';
import '../../styles/DeviceUpdatePage.css';

const DeviceUpdatePage: React.FC = () => {
  return (
    <div className="device-update-page">
      <div className="page-header">
        <h1 className="page-title">Cập nhật thiết bị</h1>
        <p className="page-description">Quản lý và cập nhật firmware cho các thiết bị IoT</p>
      </div>
      <div className="page-content">
        {/* Content will be added here */}
      </div>
    </div>
  );
};

export default DeviceUpdatePage;

