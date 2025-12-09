/**
 * Device Update Page Component
 * Page for managing device OTA updates
 */

import React, { useState, useEffect } from 'react';
import '../../styles/DeviceUpdatePage.css';
import deviceService from '../../services/deviceService';
import otaService from '../../services/otaService';
import type { DeviceWithOTA } from '../../types';

interface CheckUpdateResult {
  hasNewVersion: boolean;
  message: string;
}

interface UpdatingDevice {
  isUpdating: boolean;
  progress: number;
  isComplete?: boolean;
}

const DeviceUpdatePage: React.FC = () => {
  const [devices, setDevices] = useState<DeviceWithOTA[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [checkingUpdate, setCheckingUpdate] = useState<Record<number, boolean>>({});
  const [checkUpdateResult, setCheckUpdateResult] = useState<Record<number, CheckUpdateResult>>({});
  const [updatingDevices, setUpdatingDevices] = useState<Record<number, UpdatingDevice>>({});
  const [pollingIntervals, setPollingIntervals] = useState<Record<number, NodeJS.Timeout>>({});
  const [pollingOTAInfo, setPollingOTAInfo] = useState<Record<number, NodeJS.Timeout>>({});

  useEffect(() => {
    fetchDevicesAndOTAInfo();
  }, []);

  // Cleanup polling intervals when component unmounts
  useEffect(() => {
    return () => {
      Object.values(pollingIntervals).forEach((interval) => {
        if (interval) clearInterval(interval);
      });
      Object.values(pollingOTAInfo).forEach((interval) => {
        if (interval) clearInterval(interval);
      });
    };
  }, [pollingIntervals, pollingOTAInfo]);

  const fetchDevicesAndOTAInfo = async () => {
    try {
      setLoading(true);
      setError(null);

      // Fetch devices
      const devicesList = await deviceService.getDevices();

      // Fetch OTA info for each device
      const devicesWithOTA = await Promise.all(
        devicesList.map(async (device) => {
          if (!device.token_verify) {
            return { ...device, otaInfo: undefined, otaLoading: false };
          }

          try {
            const otaInfo = await otaService.checkOTAInfo(device.token_verify, 0);
            // Đảm bảo xử lý đúng các thuộc tính từ API
            const normalizedOtaInfo = {
              is_updating: otaInfo.is_updating ?? false,
              on_progress: typeof otaInfo.on_progress === 'number' ? otaInfo.on_progress : 0,
              auto_update: otaInfo.auto_update ?? false,
              lastVersion: otaInfo.lastVersion ?? null,
              lastUpdate: otaInfo.lastUpdate ?? null,
              currentVersion: otaInfo.currentVersion ?? null,
              hasNewVersion: otaInfo.hasNewVersion ?? false,
              error: Array.isArray(otaInfo.error) ? otaInfo.error : [],
            };

            // Start polling if currentVersion or lastVersion is still null
            if (normalizedOtaInfo.currentVersion === null || normalizedOtaInfo.lastVersion === null) {
              startPollingOTAInfo(device.id, device.token_verify);
            }

            return { ...device, otaInfo: normalizedOtaInfo, otaLoading: false, otaError: undefined };
          } catch (err) {
            return {
              ...device,
              otaInfo: undefined,
              otaLoading: false,
              otaError: err instanceof Error ? err.message : 'Failed to fetch OTA info',
            };
          }
        })
      );

      setDevices(devicesWithOTA);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to fetch devices');
    } finally {
      setLoading(false);
    }
  };

  const startPollingOTAInfo = (deviceId: number, tokenVerify: string) => {
    // Clear existing interval if any
    if (pollingOTAInfo[deviceId]) {
      clearInterval(pollingOTAInfo[deviceId]);
    }

    const interval = setInterval(async () => {
      try {
        const otaInfo = await otaService.checkOTAInfo(tokenVerify, 0);

        const normalizedOtaInfo = {
          is_updating: otaInfo.is_updating ?? false,
          on_progress: typeof otaInfo.on_progress === 'number' ? otaInfo.on_progress : 0,
          auto_update: otaInfo.auto_update ?? false,
          lastVersion: otaInfo.lastVersion ?? null,
          lastUpdate: otaInfo.lastUpdate ?? null,
          currentVersion: otaInfo.currentVersion ?? null,
          hasNewVersion: otaInfo.hasNewVersion ?? false,
          error: Array.isArray(otaInfo.error) ? otaInfo.error : [],
        };

        // Update devices state with new OTA info
        setDevices((prevDevices) =>
          prevDevices.map((device) =>
            device.id === deviceId
              ? { ...device, otaInfo: normalizedOtaInfo, otaLoading: false, otaError: undefined }
              : device
          )
        );

        // Stop polling if both currentVersion and lastVersion are no longer null
        if (normalizedOtaInfo.currentVersion !== null && normalizedOtaInfo.lastVersion !== null) {
          clearInterval(interval);
          setPollingOTAInfo((prev) => {
            const newIntervals = { ...prev };
            delete newIntervals[deviceId];
            return newIntervals;
          });
        }
      } catch (err) {
        console.error('Error polling OTA info:', err);
        // Continue polling even on error
      }
    }, 3000); // Poll every 3 seconds

    setPollingOTAInfo((prev) => ({ ...prev, [deviceId]: interval }));
  };



  const getStatusDisplay = (device: DeviceWithOTA) => {
    // Check if device is currently updating (from polling)
    const updatingDevice = updatingDevices[device.id];
    if (updatingDevice?.isUpdating) {
      return {
        text: `${updatingDevice.progress}%`,
        className: 'status-updating',
        progress: updatingDevice.progress,
      };
    }

    if (!device.otaInfo) {
      return { text: 'Đang kiểm tra ...', className: 'status-unknown' };
    }

    // Kiểm tra lỗi trước
    if (Array.isArray(device.otaInfo.error) && device.otaInfo.error.length > 0) {
      return {
        text: `Lỗi: ${device.otaInfo.error.join(', ')}`,
        className: 'status-error'
      };
    }

    if (device.otaInfo.is_updating) {
      const progress = typeof device.otaInfo.on_progress === 'number' ? device.otaInfo.on_progress : 0;
      return {
        text: `${progress}%`,
        className: 'status-updating',
        progress: progress,
      };
    }

    if (device.otaInfo.hasNewVersion) {
      return { text: 'Có bản cập nhật', className: 'status-available' };
    }

    return { text: 'Đã cập nhật', className: 'status-updated' };
  };

  const handleCheckUpdate = async (device: DeviceWithOTA) => {
    if (!device.token_verify) return;

    // Set checking state
    setCheckingUpdate((prev) => ({ ...prev, [device.id]: true }));
    setCheckUpdateResult((prev) => {
      const newResult = { ...prev };
      delete newResult[device.id];
      return newResult;
    });

    try {
      // Call check update API (type = 1)
      const otaInfo = await otaService.checkOTAInfo(device.token_verify, 1);

      // Đảm bảo xử lý đúng các thuộc tính từ API
      const hasNewVersion = otaInfo.hasNewVersion ?? false;
      const errorMessages = Array.isArray(otaInfo.error) ? otaInfo.error : [];

      if (errorMessages.length > 0) {
        setCheckUpdateResult((prev) => ({
          ...prev,
          [device.id]: {
            hasNewVersion: false,
            message: `Lỗi: ${errorMessages.join(', ')}`,
          },
        }));
      } else if (hasNewVersion) {
        setCheckUpdateResult((prev) => ({
          ...prev,
          [device.id]: {
            hasNewVersion: true,
            message: 'Có phiên bản mới khả dụng',
          },
        }));
      } else {
        setCheckUpdateResult((prev) => ({
          ...prev,
          [device.id]: {
            hasNewVersion: false,
            message: 'Thiết bị đã ở phiên bản mới nhất',
          },
        }));
      }
    } catch (err) {
      setCheckUpdateResult((prev) => ({
        ...prev,
        [device.id]: {
          hasNewVersion: false,
          message: err instanceof Error ? err.message : 'Không thể kiểm tra cập nhật',
        },
      }));
    } finally {
      setCheckingUpdate((prev) => ({ ...prev, [device.id]: false }));
    }
  };

  const startPollingProgress = (deviceId: number, tokenVerify: string) => {
    // Clear existing interval if any
    if (pollingIntervals[deviceId]) {
      clearInterval(pollingIntervals[deviceId]);
    }

    const interval = setInterval(async () => {
      try {
        const otaInfo = await otaService.checkOTAInfo(tokenVerify, 0);

        // Đảm bảo xử lý đúng các thuộc tính từ API
        const isUpdating = otaInfo.is_updating ?? false;
        const progress = typeof otaInfo.on_progress === 'number' ? otaInfo.on_progress : 0;
        const errorMessages = Array.isArray(otaInfo.error) ? otaInfo.error : [];

        setUpdatingDevices((prev) => ({
          ...prev,
          [deviceId]: {
            isUpdating: isUpdating,
            progress: progress,
          },
        }));

        // Nếu có lỗi, dừng polling và hiển thị lỗi
        if (errorMessages.length > 0) {
          clearInterval(interval);
          setPollingIntervals((prev) => {
            const newIntervals = { ...prev };
            delete newIntervals[deviceId];
            return newIntervals;
          });
          setCheckUpdateResult((prev) => ({
            ...prev,
            [deviceId]: {
              hasNewVersion: false,
              message: `Lỗi cập nhật: ${errorMessages.join(', ')}`,
            },
          }));
          setUpdatingDevices((prev) => {
            const newUpdating = { ...prev };
            delete newUpdating[deviceId];
            return newUpdating;
          });
          return;
        }

        // If update is complete (100% or not updating anymore)
        if (!isUpdating || progress >= 100) {
          clearInterval(interval);
          setPollingIntervals((prev) => {
            const newIntervals = { ...prev };
            delete newIntervals[deviceId];
            return newIntervals;
          });

          // Mark as complete and show success message
          setUpdatingDevices((prev) => ({
            ...prev,
            [deviceId]: {
              isUpdating: false,
              progress: 100,
              isComplete: true,
            },
          }));

          // Wait 2 seconds to show success message, then reload device info
          setTimeout(() => {
            fetchDevicesAndOTAInfo();
            setUpdatingDevices((prev) => {
              const newUpdating = { ...prev };
              delete newUpdating[deviceId];
              return newUpdating;
            });
            setCheckUpdateResult((prev) => {
              const newResult = { ...prev };
              delete newResult[deviceId];
              return newResult;
            });
          }, 2000);
        }
      } catch (err) {
        console.error('Error polling progress:', err);
        // Continue polling even on error
      }
    }, 2000); // Poll every 2 seconds

    setPollingIntervals((prev) => ({ ...prev, [deviceId]: interval }));
  };

  const handleUpdateNow = async (device: DeviceWithOTA) => {
    if (!device.token_verify) return;

    // Initialize updating state
    setUpdatingDevices((prev) => ({
      ...prev,
      [device.id]: {
        isUpdating: true,
        progress: 0,
      },
    }));

    // Clear check update result
    setCheckUpdateResult((prev) => {
      const newResult = { ...prev };
      delete newResult[device.id];
      return newResult;
    });

    try {
      // Trigger update (type = 1)
      await otaService.checkOTAInfo(device.token_verify, 1);

      // Start polling for progress
      startPollingProgress(device.id, device.token_verify);
    } catch (err) {
      setUpdatingDevices((prev) => {
        const newUpdating = { ...prev };
        delete newUpdating[device.id];
        return newUpdating;
      });
      setCheckUpdateResult((prev) => ({
        ...prev,
        [device.id]: {
          hasNewVersion: false,
          message: err instanceof Error ? err.message : 'Cập nhật thất bại',
        },
      }));
    }
  };

  if (loading) {
    return (
      <div className="device-update-page">
        <div className="page-header">
          <div className="page-header-left">
            <h1 className="page-title">Cập nhật thiết bị</h1>
            <p className="page-description">Quản lý và cập nhật firmware cho các thiết bị IoT</p>
          </div>
        </div>
        <div className="page-content">
          <div className="loading-container">
            <div className="spinner"></div>
            <p>Đang tải danh sách thiết bị...</p>
          </div>
        </div>
      </div>
    );
  }

  if (error) {
    return (
      <div className="device-update-page">
        <div className="page-header">
          <div className="page-header-left">
            <h1 className="page-title">Cập nhật thiết bị</h1>
            <p className="page-description">Quản lý và cập nhật firmware cho các thiết bị IoT</p>
          </div>
        </div>
        <div className="page-content">
          <div className="error-container">
            <p>{error}</p>
            <button onClick={fetchDevicesAndOTAInfo} className="btn-retry">
              Thử lại
            </button>
          </div>
        </div>
      </div>
    );
  }

  return (
    <div className="device-update-page">
      <div className="page-header">
        <div className="page-header-left">
          <h1 className="page-title">Cập nhật thiết bị</h1>
          <p className="page-description">Quản lý và cập nhật firmware cho các thiết bị IoT</p>
        </div>
        <button onClick={fetchDevicesAndOTAInfo} className="btn-refresh">
          <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
            <path d="M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8M21 8v5M21 8h-5M3 12a9 9 0 0 0 9 9 9.75 9.75 0 0 0 6.74-2.74L3 16M3 16v-5M3 16h5" />
          </svg>
          Làm mới
        </button>
      </div>
      <div className="page-content">
        {devices.filter(device => device.connection_status === 'ONLINE').length === 0 ? (
          <div className="empty-container">
            <p>Không có thiết bị nào đang online</p>
          </div>
        ) : (
          <div className="devices-table-container">
            <table className="devices-table">
              <thead>
                <tr>
                  <th>Tên thiết bị</th>
                  <th>Mã thiết bị</th>
                  <th>Vai trò</th>
                  <th>Phiên bản Firmware</th>
                  <th>Trạng thái</th>
                  <th>Thao tác</th>
                </tr>
              </thead>
              <tbody>
                {devices.filter(device => device.connection_status === 'ONLINE').map((device) => {
                  const status = getStatusDisplay(device);
                  return (
                    <tr key={device.id}>
                      <td>
                        <div className="device-name-cell">
                          <span className="device-name">{device.device_name}</span>
                        </div>
                      </td>
                      <td>
                        <span className="device-id">{device.token_verify || 'N/A'}</span>
                      </td>
                      <td>
                        <span
                          className={`role-badge ${device.device_type === 'MASTER' ? 'role-master' : 'role-slave'
                            }`}
                        >
                          {device.device_type}
                        </span>
                      </td>
                      <td>
                        <span className="firmware-version">
                          {device.otaInfo?.currentVersion === null
                            ? 'Đang kiểm tra'
                            : (device.otaInfo?.currentVersion || 'Đang kiểm tra...')}
                        </span>
                      </td>
                      <td>
                        {status.progress !== undefined ? (
                          <div className="status-progress">
                            <div className="progress-bar">
                              <div
                                className="progress-fill"
                                style={{ width: `${status.progress}%` }}
                              ></div>
                            </div>
                            <span className="progress-text">{status.text}</span>
                          </div>
                        ) : (
                          <span className={`status-badge ${status.className}`}>
                            {status.text}
                          </span>
                        )}
                      </td>
                      <td>
                        <div className="action-buttons">
                          {updatingDevices[device.id]?.isComplete ? (
                            <div className="update-success-message">
                              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                                <path d="M22 11.08V12a10 10 0 1 1-5.93-9.14" />
                                <polyline points="22 4 12 14.01 9 11.01" />
                              </svg>
                              <span>Cập nhật thành công</span>
                            </div>
                          ) : updatingDevices[device.id]?.isUpdating ? (
                            <div className="update-progress-container">
                              <div className="progress-bar-small">
                                <div
                                  className="progress-fill-small"
                                  style={{ width: `${updatingDevices[device.id].progress}%` }}
                                ></div>
                              </div>
                              <span className="progress-percentage">
                                {updatingDevices[device.id].progress}%
                              </span>
                            </div>
                          ) : checkUpdateResult[device.id] ? (
                            <>
                              {checkUpdateResult[device.id].hasNewVersion ? (
                                <button
                                  className="btn-update-now"
                                  onClick={() => handleUpdateNow(device)}
                                  disabled={!device.token_verify}
                                >
                                  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                                    <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4M17 8l-5-5-5 5M12 3v12" />
                                  </svg>
                                  <span>Cập nhật ngay</span>
                                </button>
                              ) : (
                                <div className="update-message">
                                  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                                    <path d="M22 11.08V12a10 10 0 1 1-5.93-9.14" />
                                    <polyline points="22 4 12 14.01 9 11.01" />
                                  </svg>
                                  <span>{checkUpdateResult[device.id].message}</span>
                                </div>
                              )}
                              <button
                                className="btn-check-update-secondary"
                                onClick={() => handleCheckUpdate(device)}
                                disabled={checkingUpdate[device.id] || !device.token_verify}
                              >
                                {checkingUpdate[device.id] ? (
                                  <>
                                    <div className="spinner-small"></div>
                                    <span>Đang kiểm tra...</span>
                                  </>
                                ) : (
                                  <>
                                    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                                      <path d="M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8M21 8v5M21 8h-5M3 12a9 9 0 0 0 9 9 9.75 9.75 0 0 0 6.74-2.74L3 16M3 16v-5M3 16h5" />
                                    </svg>
                                    <span>Kiểm tra lại</span>
                                  </>
                                )}
                              </button>
                            </>
                          ) : (
                            <button
                              className="btn-check-update"
                              onClick={() => handleCheckUpdate(device)}
                              disabled={checkingUpdate[device.id] || !device.token_verify}
                            >
                              {checkingUpdate[device.id] ? (
                                <>
                                  <div className="spinner-small"></div>
                                  <span>Checking...</span>
                                </>
                              ) : (
                                <>
                                  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                                    <path d="M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8M21 8v5M21 8h-5M3 12a9 9 0 0 0 9 9 9.75 9.75 0 0 0 6.74-2.74L3 16M3 16v-5M3 16h5" />
                                  </svg>
                                  <span>Kiểm tra cập nhật</span>
                                </>
                              )}
                            </button>
                          )}
                        </div>
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        )}
      </div>
    </div>
  );
};

export default DeviceUpdatePage;

