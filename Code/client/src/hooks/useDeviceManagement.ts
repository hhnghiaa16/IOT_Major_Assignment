/**
 * useDeviceManagement Hook
 * Handles device CRUD operations
 */

import { useState } from 'react';
import deviceService from '../services/deviceService';
import type { Device, RegisterDeviceRequest } from '../types';

interface UseDeviceManagementReturn {
  registerLoading: boolean;
  registerError: string | null;
  registerDevice: (deviceData: RegisterDeviceRequest) => Promise<void>;
  deleteDevice: (deviceToken: string, deviceName: string) => Promise<void>;
  getDeviceInfo: (deviceId: number) => Promise<Device>;
}

export const useDeviceManagement = (
  onDeviceChange?: () => void
): UseDeviceManagementReturn => {
  const [registerLoading, setRegisterLoading] = useState(false);
  const [registerError, setRegisterError] = useState<string | null>(null);

  const registerDevice = async (deviceData: RegisterDeviceRequest): Promise<void> => {
    setRegisterError(null);

    if (!deviceData.device_name.trim()) {
      setRegisterError('Vui lòng nhập tên thiết bị');
      return;
    }

    setRegisterLoading(true);
    try {
      await deviceService.registerDevice(deviceData);
      if (onDeviceChange) {
        await onDeviceChange();
      }
    } catch (err) {
      const errorMessage = err instanceof Error 
        ? err.message 
        : 'Lỗi khi đăng ký thiết bị';
      setRegisterError(errorMessage);
      throw err;
    } finally {
      setRegisterLoading(false);
    }
  };

  const deleteDevice = async (
    deviceToken: string,
    deviceName: string
  ): Promise<void> => {
    if (!window.confirm(`Bạn có chắc chắn muốn xóa thiết bị "${deviceName}"?`)) {
      return;
    }

    try {
      await deviceService.deleteDevice(deviceToken);
      if (onDeviceChange) {
        await onDeviceChange();
      }
      alert(`Đã xóa thiết bị "${deviceName}" thành công`);
    } catch (err) {
      const errorMessage = err instanceof Error 
        ? err.message 
        : 'Lỗi khi xóa thiết bị';
      alert(errorMessage);
      throw err;
    }
  };

  const getDeviceInfo = async (deviceId: number): Promise<Device> => {
    try {
      return await deviceService.getDevice(deviceId);
    } catch (err) {
      const errorMessage = err instanceof Error 
        ? err.message 
        : 'Lỗi khi lấy thông tin thiết bị';
      alert(errorMessage);
      throw err;
    }
  };

  return {
    registerLoading,
    registerError,
    registerDevice,
    deleteDevice,
    getDeviceInfo,
  };
};

export default useDeviceManagement;

